<?php
// $Id: memetracker.admin.inc,v 1.2 2008/08/30 13:11:17 kylemathews Exp $

/**
 * @file
 * FormAPI code for the Memetracker Admin pages.
 */

/*
 * memetracker_edit_form
 */
function memetracker_edit_form($form_state, $memetracker) {

  // Prepare default values for form.
  $content_types = array();
  $content_sources = $memetracker->get_content_sources();
  foreach ($content_sources as $content_source) {
    $content_types[$content_source] = $content_source;
  }
  
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Memetracker Name'),
    '#description' => t('Enter the name for this Memetracker'),
    '#default_value' => $memetracker->get_name(),
    '#required' => TRUE,
    '#size' => 30,
  );
  $form['pickiness'] = array(
    '#type' => 'textfield',
    '#title' => t('Pickiness'),
    '#description' => t('Enter a number between 0 and 100 for how picky 
    memetracker will be when creating memes. A higher value means memetracker 
    will be more picky about what content makes it into a meme, a lower value,
     less picky, If memes seem to have unrelated content, try increasing the 
     pickiness value.'),
    '#default_value' => abs($memetracker->get_distance_threshold()*100 - 100),
    '#required' => TRUE,
    '#size' => 30,
  );
  $form['num_memes'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of Memes'),
    '#description' => t('Enter the number of memes to be displayed'),
    '#default_value' => $memetracker->num_memes,
    '#required' => TRUE,
    '#size' => 30,
  );
  $form['num_days'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of Days of Content'),
    '#description' => t('Enter how many days of content to draw memes from'),
    '#default_value' => $memetracker->get_num_days(),
    '#required' => TRUE,
    '#size' => 30,
  );
  $form['content_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Content Types'),
    '#default_value' => $content_types,
    '#options' => array(
      'content_source_drupal_feedapi' => t('RSS Feeds'),
      'content_source_drupal_node' => t('Internal Drupal Nodes'),
    ),
    '#description' => t('Choose the types of content that can be added to your
    Memetracker.'),
  );  
  $form['old_content_types'] = array(
    '#type' => 'hidden',
    '#value' => serialize($content_types),
  );
  $form['mid'] = array(
    '#type' => 'hidden',
    '#value' => $memetracker->get_mid(),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
  );  
  return $form;
}

/*
 * Validate memetracker_edit_form
 */
function memetracker_edit_form_validate($form, &$form_state) {
  $pickiness = $form_state['values']['pickiness'];
  $num_memes = $form_state['values']['num_memes'];
  $num_days = $form_state['values']['num_days'];
  $name = $form_state['values']['name'];
  $content_types = $form_state['values']['content_types'];
  
  // Validate Memetracker Name
  if (empty($name)) {
    form_set_error('name', 'Please enter a name for the Memetracker');
  }
  // Validate distance threshold
  if ($pickiness < 0 || $pickiness > 100 || 
  !is_numeric($pickiness)) {
    form_set_error('pickiness', 'Enter a <em>number</em> between 
    0 and 100');
  }
  
  // Validate number of memes
  if (!is_numeric($num_memes) || $num_memes < 1) {
    form_set_error('num_memes', 'You must enter a positive number.');
  }
  
  // Validate number of days
  if (!is_numeric($num_days) || $num_days < 0.1) {
    form_set_error('num_memes', 'You must enter a positive number above 0.1.');
  }
  
  // Validate at least one content type is tracked
  if (!$content_types['content_source_drupal_feedapi'] 
    && !$content_types['content_source_drupal_node']) {
    form_set_error('content_types', 'You must select at least one type
    of content to follow.');
  }

}

/*
 * Submit memetracker_edit_form
 */
function memetracker_edit_form_submit($form, &$form_state) {
  $mid = $form_state['values']['mid'];
  $pickiness = $form_state['values']['pickiness'] / 100;
  $num_memes = $form_state['values']['num_memes'];
  $num_days = $form_state['values']['num_days'];
  $name = $form_state['values']['name'];  
  $old_content_types = $form_state['values']['old_content_types'];
  $content_types = $form_state['values']['content_types'];
  
  // Update values in memetracker table
  $result = db_query("UPDATE {memetracker} SET distance_threshold = %f,
  num_memes = %d, num_days = %f, name = '%s' 
  WHERE mid = %d", 
  abs($pickiness - 1), $num_memes, $num_days, check_plain($name), $mid);

  // Update Content Sources
  // check which cs are there, if there and not in new, disable
  // if not there and in new, insert
  foreach ($content_types as $content_type => $value) {
    if ($value) {
      // check if there's a row there already
      $cs = db_fetch_array(db_query("SELECT enabled 
      FROM {memetracker_content_source}
      WHERE cs_name = '%s' AND mid = %d", $content_type, $mid));
      
      // If content source row is already in database, continue.
      if ($cs['enabled']) {
        continue;
      }
      
      // If row for CS exists but is not enabled, we need to enable it.
      if (!empty($cs) && !$cs['enabled']) {
        db_query("UPDATE {memetracker_content_source} SET enabled = 1 
        WHERE cs_name = '%s' AND mid = %d", $content_type, $mid);
      }
      // If there's not a row yet, insert it.
      else{
        db_query("INSERT INTO {memetracker_content_source} VALUES (%d, '%s', 0, 
        '', 1)", $mid, $content_type);
      }
    }
    // For values equal to zero, disable the content source if it exists
    else {
      $is_there = db_result(db_query("SELECT mid FROM {memetracker_content_source}
      WHERE cs_name = '%s' AND mid = %d", $content_type, $mid));
      
      if ($is_there) {
        db_query("UPDATE {memetracker_content_source} SET enabled = 0
        WHERE cs_name = '%s' AND mid = %d", $content_type, $mid);
      }
    }
  }
  
  if ($result) {
    drupal_set_message(t("The memebrowser settings have been updated"));
  }
  else {
    form_set_error(Null, 'An error occurred updating your settings');
  }
  
  // Reset memes
  $memetracker = memetracker_load($mid);
  $memetracker->get_memes($reset_cache = True);
  
  drupal_goto('memetracker/'. $mid);
}

/*
 * memetracker_edit_feed_list_form
 */
function memetracker_edit_feed_list_form($form_state, $memetracker) {
  // Query for feeds being used already
  $data = db_result(db_query("SELECT data FROM {memetracker_content_source}
  WHERE mid = %d AND cs_name = 'content_source_drupal_feedapi'", 
  $memetracker->get_mid()));
  
  $data = unserialize($data);
  if (empty($data)) {
    $data = array();
  }

  // Query list of Feeds available through feedapi
  $results = db_query("SELECT f.nid, n.title FROM {feedapi} f JOIN {node} n
  WHERE f.nid = n.nid");
  
  // Loop through results and assemble an array to set as options in the form
  $options = array();
  while ($feed = db_fetch_array($results)) {
    $options[$feed['nid']] = $feed['title'];
  }
  $form['feeds'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Feeds'),
    '#default_value' => $data,
    '#options' => $options,
    '#description' => t('Select feeds to include in this memetracker'),
  );
  $form['mid'] = array(
    '#type' => 'hidden',
    '#value' => $memetracker->get_mid(),
  );
  $form['old_feed_list'] = array(
    '#type' => 'hidden',
    '#value' => serialize($data),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
  );  
  
  return $form;
}

/*
 * Validate memetracker_edit_form
 */
function memetracker_edit_feed_list_form_validate($form, &$form_state) {
  return;
}

/*
 * Submit memetracker_edit_form
 */
function memetracker_edit_feed_list_form_submit($form, &$form_state) {
// eventually I'll need to check if a source is being removed and for all sources
// that are removed, remove their accompanying content in memetracker_content

  $mid = $form_state['values']['mid'];
  $memetracker = memetracker_load($mid);

  $old_feed_list = unserialize($form_state['values']['old_feed_list']);
  
  $checkboxes = $form_state['values']['feeds'];
  $feeds = array();
  // assemble array to insert into the database
  foreach ($checkboxes as $check) {
    // check not zero
    if ($check) {
      $feeds[] = $check;
    }
  }
  
  // Update feed list in database.
  $result = db_query("UPDATE {memetracker_content_source} SET data = '%s' WHERE 
  mid = %d", serialize($feeds), $mid);
  
  if ($result) {
    drupal_set_message(t("The memebrowser settings have been updated"));
  }
  else {
    form_set_error(Null, 'An error occurred updating your settings');
  }
  
  // Get array of feeds that are in the old list but not in the new list of 
  // feeds. Which, in other words, were removed from the list.
  $deleted_feeds = array_diff($old_feed_list, $feeds);
  if (!empty($deleted_feeds)) {
    $deleted_feeds_string = array_to_sql_string($deleted_feeds);

    // Delete content in memetracker_content that are part of deleted feeds
    db_query("DELETE m 
              FROM {memetracker_content} m JOIN {feedapi_node_item_feed} f 
              WHERE f.feed_item_nid = m.int_id 
              AND f.feed_nid IN ($deleted_feeds_string) 
              AND mid = %d", $memetracker->get_mid());
  }
  
  // Delete items from machinelearningapi_distance_cache
  
  // Get content for new feeds and reset memes
  $memetracker->check_new_content();
  
  $memetracker->get_memes($reset_cache = True);
  
  drupal_goto('memetracker/'. $mid);
}

/*
 * Refresh memetracker so check for new content and recalculate memes
 */
function memetracker_refresh($memetracker) {
  // Check for new content
  $memetracker->check_new_content();
  
  // Recalculate memes
  $memetracker->get_memes($reset_cache = True);
  
  // Return to memebrowsing page
  drupal_goto('memetracker/'. $memetracker->get_mid());
}

/*
 * Form for creating new Memetrackers.
 */
function memetracker_new_form($form_state) {
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Memetracker Name'),
    '#description' => t('Enter the name for this Memetracker'),
    '#required' => TRUE,
    '#size' => 30,
  );
  $form['pickiness'] = array(
    '#type' => 'textfield',
    '#title' => t('Pickiness'),
    '#description' => t('Enter a number between 0 and 100 for how picky 
    memetracker will be when creating memes. A higher value means memetracker 
    will be more picky about what content makes it into a meme, a lower value,
     less picky, If memes seem to have unrelated content, try increasing the 
     pickiness value.'),
    '#required' => TRUE,
    '#size' => 30,
  );
  $form['num_memes'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of Memes'),
    '#description' => t('Enter the number of memes to be displayed'),
    '#required' => TRUE,
    '#size' => 30,
  );
  $form['num_days'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of Days of Content'),
    '#description' => t('Enter how many days of content to draw memes from'),
    '#required' => TRUE,
    '#size' => 30,
  );
  $form['content_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Content Types'),
    '#options' => array(
      'content_source_drupal_feedapi' => t('RSS Feeds'),
      'content_source_drupal_node' => t('Internal Drupal Nodes'),
    ),
    '#description' => t('Choose the types of content that can be added to your
    Memetracker.'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
  );  
  return $form;
}

/*
 * Validate memetracker_edit_form
 */
function memetracker_new_form_validate($form, &$form_state) {
  $pickiness = $form_state['values']['pickiness'];
  $num_memes = $form_state['values']['num_memes'];
  $num_days = $form_state['values']['num_days'];
  $name = $form_state['values']['name'];
  $content_types = $form_state['values']['content_types'];
  
  // Validate Memetracker Name
  if (empty($name)) {
    form_set_error('name', 'Please enter a name for the Memetracker');
  }
  // Validate pickiness
  if ($pickiness < 0 || $pickiness > 100 || 
  !is_numeric($pickiness)) {
    form_set_error('pickiness', 'Enter a <em>number</em> between 
    0 and 100');
  }
  
  // Validate number of memes
  if (!is_numeric($num_memes) || $num_memes < 1) {
    form_set_error('num_memes', 'You must enter a positive number.');
  }
  
  // Validate number of days
  if (!is_numeric($num_days) || $num_days < 0.1) {
    form_set_error('num_days', 'You must enter a positive number above 0.1.');
  }
  
  // Validate at least one content type is tracked
  if (!$content_types['content_source_drupal_feedapi'] 
  && !$content_types['content_source_drupal_node']) {
    form_set_error('content_types', 'You must select at least one type
    of content to follow.');
  }
}

/*
 * Submit memetracker_edit_form
 */
function memetracker_new_form_submit($form, &$form_state) {
  $pickiness = $form_state['values']['pickiness'] / 100;
  $num_memes = $form_state['values']['num_memes'];
  $num_days = $form_state['values']['num_days'];
  $name = $form_state['values']['name'];  
  $content_types = $form_state['values']['content_types'];
  
  // Insert values in memetracker table.
  $result1 = db_query("INSERT INTO {memetracker} VALUES ('', 
  'simple_classifier1', '0', %f, %d, %f, '%s')",
  abs($pickiness - 1), $num_memes, $num_days, check_plain($name));
  
  // Retrive the mid of the new Memetracker.
  $mid = db_last_insert_id('memetracker', 'mid');
  
  // Insert content_source rows for the new Memetracker.
  foreach ($content_types as $content_type => $value) {
    // Insert only if checkbox selected.
    if ($value) {
      db_query("INSERT INTO {memetracker_content_source} 
      VALUES (%d, '%s', 0, '', 1)", $mid, $content_type);
    }
  }
  
  if ($result1) {
    drupal_set_message(t("Your new memetracker is created. Click the \"Edit Feed
    List\" tab to select feeds for your new memetracker to track."));
  }
  else {
    form_set_error(Null, 'An error occurred creating your memetracker');
  }

  // Go to new memebrowser page
  drupal_goto('memetracker/'. $mid);
}

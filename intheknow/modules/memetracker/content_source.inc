<?php
// $Id: content_source.inc,v 1.11 2008/08/30 13:11:17 kylemathews Exp $

/**
 * @file
 * The theme system, which controls the output of Drupal.
 *
 * The theme system allows for nearly all output of the Drupal system to be
 * customized by user themes.
 */

require_once 'memetracker.inc';
require_once 'content.inc';


/**
 * class content_source
 */
interface content_source {

  /** Aggregations: */

  /** Compositions: */
#  var $m_2;

  public function __construct($mid, $num_days, $cs_name = Null);
  
  public function save();
  /**
   *
   * @return content
   * @access public
   */
  public function get_content_array( );


} // end of content_source

/**
 * class content_source_drupal
 */
abstract class content_source_drupal implements content_source {

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/
  private $mid;
  public $last_cron;
  public $cs_name;
  
  public function __construct($mid, $num_days, $cs_name = Null) {
    $this->cs_name = $cs_name;
    $this->mid = $mid;
    $this->num_days = $num_days;
    $array = db_fetch_array(db_query("SELECT last_cron, data FROM
    {memetracker_content_source} WHERE cs_name = '%s' and mid = %d",
     $this->cs_name, $this->mid));
    $this->last_cron = $array['last_cron'];
    $this->data = $array['data'];
    
  }
  
  public function get_mid() {
    return $this->mid;
  }
    
  public function save() {
    db_query("UPDATE {memetracker_content_source} SET last_cron = %d
      WHERE cs_name = '%s' AND mid = %d", $this->last_cron, $this->cs_name,
      $this->mid);
  }
  /**
   *
   * @return content
   * @access public
   */
  public function get_content_array() {

  } // end of member function get_content


} // end of content_source_drupal


/**
 * class content_source_drupal_nodes
 */
class content_source_drupal_node extends content_source_drupal {

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/
  
  public function __construct($mid, $num_days, $cs_name = Null) {
    parent::__construct($mid, $num_days, 'content_source_drupal_node');
  }
  
  /**
   *,
   * @return content
   * @access public
   */
  public function get_content_array() {
    // select new nodes which have been created -- at some point this will be configurable
    // so user can select what type of nodes it can create or even use a view
    $nids = db_query("SELECT n.nid, n.created, u.uid, u.name 
    FROM {node} n 
    JOIN {users} u
    WHERE n.created > %d AND n.uid = u.uid
    AND n.type not in ('feedapi_node', 'feed_item')", 
    $this->last_cron);
    
    sleep(1);
    $this->last_cron = time();
    $this->save();
    
    $content_array = Null;
    
    // create content objects
    $counter = 0;
    while ($nid = db_fetch_array($nids)) {
#      dpm('drupal nid '. $nid['nid']);
      $content_array[] = new content_drupal_node($this->get_mid(), NULL, $nid['nid'],
      Null, $nid['created']);
      $counter++;
    }
    
    if ($counter > 0) {
      drupal_set_message("Content Source Node successfully added ". $counter .
      " new content");
    }
    return $content_array;
    
  } // end of member function get_content


} // end of content_source_drupal_node

/**
 * class content_source_drupal_feed
 * pulls in feedapi items, 
 */
class content_source_drupal_feedapi extends content_source_drupal {

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/
  
  public function __construct($mid, $num_days, $cs_name = Null) {
    parent::__construct($mid, $num_days, 'content_source_drupal_feedapi');
  }
    
  /**
   *
   * @return content
   * @access public
   */
  public function get_content_array() {
    // need something more reliable then querying node table
    // is it possible to hook into the feedapi module directly?

    $data = unserialize($this->data);
    $data_str = array_to_sql_string($data);
    if (empty($data_str)) {
      $data_str = "0"; // So MySQL doesn't complain.
    }
    // Select new nodes brought in by feedapi.
    // Check against memetracker content table for duplicate content as some
    // feed items' timestamps are in the future meaning each cron run until
    // we catch up, content_source_drupal_feedapi will grab the same content 
    // again.
    $nids = db_query("SELECT fni.nid, fni.timestamp, f.link, f.nid as fid
    FROM {feedapi} f 
    JOIN {feedapi_node_item_feed} ff
    JOIN {feedapi_node_item} fni
    WHERE fni.timestamp > %d AND f.nid in ($data_str) AND
    f.nid = ff.feed_nid AND ff.feed_item_nid = fni.nid
    AND fni.nid NOT IN 
    (SELECT int_id FROM {memetracker_content} 
    WHERE mid = %d)", time() - $this->num_days * 86400, $this->get_mid());

    sleep(1);
    $this->last_cron = time();
    $this->save();
    
    $content_array = Null;
    
    // create content objects
    $counter = 0;
    while ($nid = db_fetch_array($nids)) {
#      dpm('drupal nid '. $nid['nid']);
      $title = db_result(db_query("SELECT title FROM {node} WHERE nid = %d", 
      $nid['fid']));
     
            
      $content_array[] = new content_drupal_feedapi($this->get_mid(), NULL, 
      $nid['nid'], Null, $nid['timestamp']);

      $counter++;
    }

    if ($counter > 0) {
      drupal_set_message("Content Source Feedapi successfully added ". $counter .
      " new content");
    }
    return $content_array;
    
  } // end of member function get_content



} // end of content_source_drupal_feedapi

#class content_source_drupal_aggregator extends content_source_drupal {

#  /** Aggregations: */

#  /** Compositions: */

#   /*** Attributes: ***/
#  
#  public function __construct($mid, $cs_name = Null) {
#    parent::__construct($mid, 'content_source_drupal_aggregator');
#  }
#  
#  /**
#   *
#   * @return content
#   * @access public
#   */
#  public function get_content_array() {
#    $data = unserialize($this->data);
#    $data_str;
#    foreach ($data as $d) {
#      $data_str .= '\''. $d .'\',';
#    }
#    $data_str = trim($data_str, ',');

#    // select new items brought in by feedapi
#    $iids = db_query("SELECT ai.iid, ai.timestamp, af.title, af.link 
#    FROM {aggregator_item} ai 
#    JOIN {aggregator_feed} af 
#    WHERE ai.timestamp > %d AND ai.fid = af.fid and ai.fid in ($data_str)",
#     $this->last_cron); // how do I do this query the 'Drupal' way?
#    
#    $this->last_cron = time();
#    $this->save();
#    $content = Null;
#    // create content objects
#    $counter = 0;
#    while ($iid = db_fetch_array($iids)) {
#      $source = l($iid['title'], $iid['link']);
#      $content[] = new content_drupal_aggregator($this->get_mid(), Null, $iid['iid'],
#      Null, $iid['timestamp'], $source);
#      $counter ++;
#    }
#    if ($counter > 0) {
#      drupal_set_message("The Aggregator content source successfully added ". $counter ." new content.");
#    }
#    return $content;
#    
#  } // end of member function get_content



#} // end of content_source_drupal_aggregator

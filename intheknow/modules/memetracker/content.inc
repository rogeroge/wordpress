<?php
// $Id: content.inc,v 1.8 2008/08/30 13:11:17 kylemathews Exp $

/**
 * @file
 * The theme system, which controls the output of Drupal.
 *
 * The theme system allows for nearly all output of the Drupal system to be
 * customized by user themes.
 */

require_once 'memetracker.inc';
require_once 'click_data.inc';


/**
 * class content
 */
interface content {

  /** Aggregations: */

  /** Compositions: */

  public function __construct($mid, $cid = Null, $int_id = Null, $content_type = Null,
  $timestamp = Null);

  /**
   *  Returns an object that implements content
   *
   * @return content object
   * @access public
   */
  public function get_content_obj();

  /**
   *  Returns an link to the content wherever that might be
   *
   * @return url
   * @access public
   */
  public function get_link();
   
  /**
   *
   * @return int
   * @access public
   */
  public function get_cid( );

  /**
   *
   * @return string
   * @access public
   */
  public function get_title( );

  /**
   *
   * @return string
   * @access public
   */
  public function get_body( );
  
   /**
   *
   * @return string
   * @access public
   */
  public function get_content_type();

  /**
   *
   * @return string
   * @access public
   */
  public function get_content();
  
  /**
   *
   * @return 
   * @access public
   */
  public function get_timestamp( );

  /**
   *
   * @return string
   * @access public
   */
  public function get_author( );

  /**
   *
   * @return string
   * @access public
   */
  public function get_source( );

  /**
   *
   * @return Calias
   * @access public
   */
  public function get_tags( );

  /**
   *
   * @return click_data
   * @access public
   */
  public function get_click_data($last_cron);


  public function get_interestingness();

  public function set_interestingness($interestingness);

  /**
   *
   * @return 
   * @access public
   */
  public function save( );

  /*
   * returns array of href which content links to, comments only link to orginal post
   */
  public function get_links();
} // end of content





/**
 * class content_drupal
 */
abstract class content_drupal implements content {

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/
  public $cid;
  private $mid;
  private $interestingness = Null;
  private $content_type;
  private $timestamp;
  
  public function __construct($mid, $cid = Null, $int_id = Null, $content_type = Null,
  $timestamp = Null) {
    $this->int_id = $int_id;
    $this->cid = $cid;
    $this->mid = $mid;
    $this->content_type = $content_type;
    $this->timestamp = $timestamp;
    // If we don't know yet the content id (i.e. we're just now creating it), save
    // the content object to the database
    if (is_null($cid)) {
      $this->save();
    }
    
    // If cid is set, i.e. content was saved previously, 
    // pull needed information from the database
    if (!is_null($cid)) {
      $array = db_fetch_array(db_query("SELECT timestamp, content_type,
        interestingness FROM {memetracker_content} WHERE cid = %d", $cid));
    
      $this->content = db_result(db_query("SELECT content FROM 
      {memetracker_search_%d} WHERE cid = %d", $this->mid, $cid));
    
      $this->timestamp = $array['timestamp'];
      $this->content_type = $array['content_type'];
      $this->interestingness = $array['interestingness'];
      $this->source = $array['source'];
    }
  }
  
  /**
   *  Returns an link to the content wherever that might be
   *
   * @return url
   * @access public
   */
  public function get_link() {
  
  }
  
  public function get_content_obj() {
  
  }
  
  public function get_mid() {
    return $this->mid;
  }

  /**
   *
   * @return int
   * @access public
   */
  public function get_cid() {
    return $this->cid;
  } // end of member function get_cid
  
  /*
   * Return the Drupal internal id -- so the node id or item id or video id or whatever
   */
  public function get_int_id() {
    return $this->int_id;
  }
  
  /**
   *
   * @return string
   * @access public
   */
  public function get_title() {

  } // end of member function get_title

  /**
   *
   * @return string
   * @access public
   */
  public function get_body() {

  } // end of member function get_body

  /**
   *
   * @return string
   * @access public
   */
  public function get_content_type() {
    return $this->content_type;
    
  } // end of member function get_content_type
  
  /**
   *
   * @return string
   * @access public
   */
  public function get_content() {
    return $this->content;
  } // end of member function get_content
  
  public function get_interestingness() {
    return $this->interestingness;
  }

  public function set_interestingness($interestingness) {
    $this->interestingness = $interestingness;
    db_query("UPDATE {memetracker_content} SET interestingness = %f
    WHERE cid = %d", $interestingness, $this->cid);
  }
  
  /**
   *
   * @return 
   * @access public
   */
  public function get_timestamp() {
    return $this->timestamp;
  } // end of member function get_timestamp

  /**
   *
   * @return string
   * @access public
   */
  public function get_author() {

  } // end of member function get_author

  /**
   *
   * @return string
   * @access public
   */
  public function get_source() {

  } // end of member function get_source

  /** 
   *
   * @return Calias
   * @access public
   */
  public function get_tags() {
    return Null; // this is a placeholder for now
  } // end of member function get_tags

  /**
   *
   * @return click_data
   * @access public
   */
  public function get_click_data($last_cron) {
    return db_result(db_query("SELECT count(cid) FROM {memetracker_clicks}
    WHERE mid = %d AND cid = %d AND timestamp > %d", 
    $this->mid, $this->get_cid(), $last_cron));
  } // end of member function get_click_data

  
  public function get_links() {
    // TODO pull code from http://drupal.org/project/feedapi_node_discussion -- remember to post thank you somewhere
    // Drupal comments are first class content objects, they have link to orginal post
  }
  
  /*
   * Grabs title and body, sanatizes them of HTML tags or any other nasties and 
   * returns them as a string
   */
  public function clean_content($content) {
    $content = strip_tags($content);
    // remove stop words as well from here at some point. . . use mysql fulltext search list
    
    return $content;
  }
  
  public function save() {
    // clean up content
    $this->content = $this->clean_content($this->get_title() .' '. 
    $this->get_body());
    
    // Build array to pass into drupal_write_records
    $array = array('mid' => $this->mid, 'cid' => '', 
    "int_id" => $this->get_int_id(), "content_type" => $this->content_type, 
    "links" => '', 'timestamp' => $this->timestamp,
    "source" => $this->source);
    
    drupal_write_record('memetracker_content', $array);

    // drupal_write_record retrieves the new id set by the database, we set it here
    $this->cid = $array['cid'];

    // Insert content.
    db_query("INSERT INTO {memetracker_search_%d} (cid, content) VALUES 
    (%d, '%s')", $this->mid, $this->cid, $this->content);
    

    
  } // end of member function save_content
  
} // end of content_drupal


/**
 * class content_drupal_node
 */
class content_drupal_node extends content_drupal {

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/
  private $node = Null;
  
  public function __construct($mid, $cid = Null, $int_id = Null, $content_type = Null,
    $timestamp = Null, $source = Null) {
    // call parent constructor in content_drupal
    parent::__construct($mid, $cid, $int_id, 'content_drupal_node', $timestamp, $source);
    
    // If the content object is already saved to database, when a content object
    // is created, we will know the content id and not the internal id
    if (is_null($int_id) AND !is_null($cid)) {
      // Query for nid int_id = internal id so the node id or aggregator id or whatever else
      $nid = db_result(db_query("SELECT int_id FROM {memetracker_content} WHERE
      cid = %d}", $cid));
      $this->int_id = $nid;
    }
  }
  
  /*
   * Singleton pattern
   */
  public function get_node_instance() {
    if (is_null($this->node)) {
      return node_load(parent::get_int_id());
    }
    else {
      return $this->node;
    }
  }
  
  public function get_content_obj() {
    return $this->get_node_instance();
  }
  
  /**
   *
   * @return string
   * @access public
   */
  public function get_title() {
    return $this->get_node_instance()->title;
  } // end of member function get_title

  /**
   *
   * @return string
   * @access public
   */
  public function get_body() {
    return $this->get_node_instance()->body;
  } // end of member function get_body

  /**
   *  Returns an link to the content wherever that might be
   *
   * @return url
   * @access public
   */
  public function get_link() {
    return 'node/'. $this->get_int_id();
  }
  
  /**
   *
   * @return string
   * @access public
   */
  public function get_author() {
    return $this->get_source_name(); // Author of Drupal node equals source name
  } // end of member function get_author

  /**
   *
   * @return string
   * @access public
   */
  public function get_source_name() { //       $source = l($nid['name'], 'user/'. $nid['uid']); // use theme function for displaying person // get_author is duplicate of this
    return "Need to Implement still.";
  } // end of member function get_source

  public function get_source_link() {
    return "Need to Implement still.";
  }
} // end of content_drupal_node


/**
 * class content_drupal_feed
 * Used for content brought in by FeedAPI and stored in table feedapi_aggregator_item
 */
class content_drupal_feedapi extends content_drupal {

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/
  private $item = Null;
  
  
  public function __construct($mid, $cid = Null, $int_id = Null, $content_type = Null,
    $timestamp = Null, $source = Null) {
    // call parent constructor in content_drupal
    parent::__construct($mid, $cid, $int_id, 'content_drupal_feedapi', $timestamp, $source);
    
    // If the content object is already saved to database, when a content object
    // is created, we will know the content id and not the internal id
    if (is_null($int_id) AND !is_null($cid)) {
      // Query for nid int_id = internal id so the node id or aggregator id or whatever else
      $iid = db_result(db_query("SELECT int_id FROM {memetracker_content} WHERE
      cid = %d", $cid));
      $this->int_id = $iid;
    }
  }
  
  /*
   * Singleton pattern
   */
  private function get_item_instance() {
    if (is_null($this->item)) {
      $this->item = node_load($this->int_id);
      
      return $this->item;
    }
    else {
      return $this->item;
    }
  }

  public function get_content_obj() {
    return $this->get_item_instance();
  }
      
  /**
   *
   * @return string
   * @access public
   */
  public function get_title() {
    return $this->get_item_instance()->title;
  } // end of member function get_title

  /**
   *
   * @return string
   * @access public
   */
  public function get_body() {
    return $this->get_item_instance()->body;
  } // end of member function get_body

  /**
   *  Returns an link to the content wherever that might be
   *
   * @return url
   * @access public
   */
  public function get_link() {
    return db_result(db_query("SELECT url FROM {feedapi_node_item}
    WHERE nid = %d", $this->int_id));
  }
  
  /**
   *
   * @return string
   * @access public
   */
  public function get_author() {
    return $this->get_item_instance()->author; // BIG PROBLEM HERE -- Drupal Node name only works for nodes created within Drupal -- feedapi sets author of feed node
  } // end of member function get_author

  /**
   *
   * @return string
   * @access public
   */
  public function get_source_name() {
    $node = $this->get_content_obj();
    $feednode = $node->feedapi_node;
    $feed_nids = array_keys($feednode->feed_nids);
    $name = db_result(db_query("SELECT title FROM {node} WHERE nid = %d", 
    $feed_nids[0]));
    return $name;
  } // end of member function get_source

  public function get_source_link() {
    $node = $this->get_content_obj();
    $feednode = $node->feedapi_node;
    $feed_nids = array_keys($feednode->feed_nids);
    $result = db_fetch_array(db_query("SELECT n.title, f.link 
    FROM {node} n JOIN {feedapi} f
    WHERE n.nid = f.nid AND n.nid = %d", $feed_nids[0]));
    
    return l($result['title'], $result['link']);
  }
} // end of content_drupal_feedapi_light

/**
 * class content_drupal_aggregator
 * Used for content brought in by the core aggregator and stored in table aggregator_item
 */
class content_drupal_aggregator extends content_drupal {

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/
  private $item = Null;
  
  
  public function __construct($mid, $cid = Null, $int_id = Null, $content_type = Null,
    $timestamp = Null, $source = Null) {
    // call parent constructor in content_drupal
    parent::__construct($mid, $cid, $int_id, 'content_drupal_aggregator', $timestamp, $source);
    
    // If the content object is already saved to database, when a content object
    // is created, we will know the content id and not the internal id
    if (is_null($int_id) AND !is_null($cid)) {
      // Query for nid int_id = internal id so the node id or aggregator id or whatever else
      $iid = db_result(db_query("SELECT int_id FROM {memetracker_content} WHERE
      cid = %d", $cid));
      $this->int_id = $iid;
    }
  }
  
  /*
   * Singleton pattern
   */
  private function get_item_instance() {
    if (is_null($this->item)) {
      $this->item = db_fetch_object(db_query("SELECT * FROM {aggregator_item}
      where iid = %d", $this->int_id));
      
      return $this->item;
    }
    else {
      return $this->item;
    }
  }
  
  public function get_content_obj() {
    return $this->get_item_instance();
  }
  
  /**
   *
   * @return string
   * @access public
   */
  public function get_title() {
    return $this->get_item_instance()->title;
  } // end of member function get_title

  /**
   *
   * @return string
   * @access public
   */
  public function get_body() {
    return $this->get_item_instance()->description;
  } // end of member function get_body

  /**
   *  Returns an link to the content wherever that might be
   *
   * @return url
   * @access public
   */
  public function get_link() {
    return $this->get_item_instance()->link;
  }

  /**
   *
   * @return string
   * @access public
   */
  public function get_author() {
    return $this->get_item_instance()->author; // BIG PROBLEM HERE -- Drupal Node name only works for nodes created within Drupal -- feedapi sets author of feed node
  } // end of member function get_author

  /**
   *
   * @return string
   * @access public
   */
  public function get_source() {
    return $this->source;
  } // end of member function get_source

} // end of content_drupal_aggregator

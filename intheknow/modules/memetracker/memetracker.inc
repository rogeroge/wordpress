<?php
// $Id: memetracker.inc,v 1.8 2008/08/30 13:11:17 kylemathews Exp $

/**
 * @file
 * Controller for Memetracker system. 
 *
 * Retrives new content via content_source classes. 
 * Gets memes via machinelearning_api module.
 */


include_once 'machine_learning_api.inc';
include_once 'meme.inc';
include_once 'content.inc';
include_once 'content_source.inc';


/**
 * class Memetracker
 */
class Memetracker {

  /** Aggregations: */

  /** Compositions: */
  var $m_;

   /*** Attributes: ***/
  
  // Memetracker ID
  private $mid;
  private $content_sources = array();
  private $content_array = array();
  private $ml_api_obj;
  private $ml_api_name;
  private $last_cron;
  private $distance_threshold;
  public $num_memes;
  private $num_days;
  public $cutoff_point;
  private $name;
  
  /*
   * Constructor for Memetracker
   */
  function __construct($mid) {
    $result = db_result(db_query("SELECT mid FROM {memetracker} 
    WHERE mid = %d", $mid));
    // Only build memetracker if there's an already existing memetracker 
    // in the database.
    if ($result == $mid) {
      $this->mid = $mid;
      $tw = time() - 86400 * 2; 

      // Load info about memetracker from the database.
      $results = db_fetch_array(db_query("
      SELECT ml_api, last_cron, distance_threshold, num_memes, num_days, name
      FROM {memetracker} 
      WHERE mid = %d", $mid));
      $cs_names = db_query("SELECT cs_name from {memetracker_content_source}
      WHERE mid = %d AND enabled = 1", $mid);
      
      $this->cutoff_point = time() - (86400 * $results['num_days']);

      $content = db_query("SELECT cid, content_type FROM {memetracker_content}
        WHERE mid = %d and timestamp > %d ORDER BY cid", 
        $mid, $this->cutoff_point);

      // Load names of content source objects
      $content_sources = array();
      while ($data = db_fetch_array($cs_names)) {
        $content_sources[] = $data['cs_name'];
      }
      
      // Load array of content objects
      $content_array = array();
      while ($data = db_fetch_array($content)) {
        $content_class_string = $data['content_type'];
        $content_array[] = new $content_class_string($this->get_mid(), $data['cid'], Null);
      }
      
      // Set values of variables pulled from the database
      $this->ml_api_name = $results['ml_api'];
      $this->last_cron = $results['last_cron'];
      $this->distance_threshold = $results['distance_threshold'];
      $this->num_memes = $results['num_memes'];
      $this->num_days = $results['num_days'];
      $this->name = $results['name'];
      $this->set_content_sources($content_sources);
      $this->set_content_array($content_array);
    }
  }
  
  /*
   * Set content_sources
   */
  public function set_content_sources($content_sources) {
    $this->content_sources = $content_sources;
  }
  
  /*
   * Get content_sources
   */
  public function get_content_sources() {
    return $this->content_sources;
  }
  /*
   * Set content_array 
   */
  public function set_content_array($content_array) {
    $this->content_array = $content_array;
  }
  
  /*
   * Get Memetracker ID
   */
  public function get_mid() {
    return $this->mid;
  }
  
  /*
   * Get num_days
   */
  function get_num_days() {
    return $this->num_days;
  }
  /*
   * Get distance_threshold
   */
  function get_distance_threshold() {
    return $this->distance_threshold;
  }
  
  /*
   * Get Name
   */
  function get_name() {
    return $this->name;
  }
  /*
   * Singleton pattern to return instance of machine learner
   */
  private function get_ml_instance() {
    if (is_null($this->ml_api_obj)) {
      $this->ml_api_obj = new $this->ml_api_name($this->mid);
      return $this->ml_api_obj;
    }
    else {
      return $this->ml_api_obj;
    }
  }
  
  /*
   * Use for testing only
   */
  public function test_get_ml_instance() {
    return $this->get_ml_instance();
    
  }
  
  public function load_content_source($class_name) {  
    if (is_string($class_name)) {
      return new $class_name($this->mid, $this->num_days);
    }
  }
  /*
  * Cleanup routine for cron run
  * Delete content older than 2 * num_days limit so search index is 
  * relavent as possible (still thinking about this one -- not implemented for now)
  * Also cleans up old distance scores from the 
  * machinelearningapi_distance_cache.
  */
  public function cleanup() {
    // Select cids of soon to be deleted content so we can delete from the 
    // distance_cache.
    $results = db_query("SELECT cid FROM {memetracker_content} 
    WHERE mid = %d AND timestamp > %d", 
    $this->mid, $this->cutoff_point - $this->num_days);
    
    // Delete old content from memetracker_content.
    db_query("DELETE FROM {memetracker_content} WHERE mid = %d AND timestamp > 
    %d", $this->mid, $this->cutoff_point - $this->num_days);
    
#    // Delete old content from memetracker_search_N
#    db_query("DELETE s FROM {memetracker_search_%d} s JOIN {memetracker_content} m
#    WHERE m. timestamp > %d", 
#    $this->mid, $this->cutoff_point - $this->num_days);
    
    // Delete distances from machinelearningapi_distance_cache.
    $array;
    while ($data = db_fetch_array($results)) {
      $array[] = $data['cid'];
    }
    
    $cids = array_to_sql_string($array);
    
    if (!empty($cids)) {
      db_query("DELETE FROM {machinelearningapi_distance_cache} WHERE cid1 IN 
      ($cids) OR cid2 in ($cids)");
    }
  }
  
  /**
   * Loops through active content_sources calling their get_content_array() 
   * method. All new content is saved as created and merged with the existing
   * array of content objects.
   *
   * @access public
   */
  public function check_new_content( ) {
    // new content count
    $count;
#    // Reload content_array.
#    $content = db_query("SELECT cid, content_type FROM {memetracker_content}
#      WHERE mid = %d and timestamp > %d", $mid, $this->cutoff_point);
    
#    // Load array of content objects
#    $content_array = array();
#    while ($data = db_fetch_array($content)) {
#      $content_class_string = $data['content_type'];
#      $content_array[] = new $content_class_string($this->get_mid(), $data['cid'], Null);
#    }

#    $this->set_content_array($content_array);
    
    // loop through $content_sources calling get_content on each.
    foreach ($this->content_sources as $content_source_string) {
      $cs_obj = $this->load_content_source($content_source_string);
      
      // If this is the first time checking for content, set last_cron to the
      // number of days memetracker tracks.
      if ($cs_obj->last_cron == 0) {
        $cs_obj->last_cron = time() - ($this->num_days * 86400);
      }
      
      $new_content = $cs_obj->get_content_array();
      
      // If the array is null, create an empty array so the array_merge function
      // doesn't complain.
      if (is_null($new_content)) {
        $new_content = array();
      }

      // add new content objects to existing content objects
      $this->content_array = array_merge($this->content_array, $new_content);
      
      $count += count($new_content);
    }
    
    return $count;
  } // end of member function check_new_content

  /**
   * Returns array of memes. If $reset_cache is set to false, memes are retrived
   * from cache. If $reset_cache is true, then the machine_learning_api 
   * recalculates memes.
   *
   * @param content content_array 
   * @return array array of memes
   * @access public
   */
  public function get_memes($reset_cache = False) {
    if ($reset_cache || 
    empty(cache_get('memetracker_memes'. $this->mid)->data)) {
      $ml_api = $this->get_ml_instance();
      
      // Set interestingness score on content objects 
      // if they don't have one already.
      $ml_api->calculate_content_interestingness($this->content_array);
      
      // Get memes from machine_learning_api.
      $memes = $ml_api->get_memes($this->content_array, 
      $this->distance_threshold, $this->num_days);

      cache_set('memetracker_memes'. $this->mid, $memes);
      return $memes;
    }
    else {
      $memes = cache_get('memetracker_memes'. $this->mid)->data;
    }      
    
    if ($memes) {
      array_splice($memes, $this->num_memes);
    }
    
    return $memes;
    
  } // end of member function get_memes
  
  /**
   * Trains naive_bayes. Memetracker records which content is clicked on from
   * the memebrowsing page. For each clicked-on content, naive_bayes is trained
   * with that content as "interesting".
   */
  public function train_on_click_data() {
    $ml_api = $this->get_ml_instance(); // get_machine_learner
    $ml_api->train_on_click_data($this->content_array, $this->last_cron);
  }
  
  /*
   * Saves time of cron run to memetracker table
   */
  function save_cron_time() {
    $this->last_cron = time();
    db_query("UPDATE {memetracker} SET last_cron = %d WHERE mid = %d", 
    time(), $this->mid);
  }
}

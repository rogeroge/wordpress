<?php
// $Id: machine_learning_api.inc,v 1.1 2008/08/30 13:11:17 kylemathews Exp $

/**
 * @file
 * Provides interface for machinelearningapi
 *
 * Implementing classes provide functions that get memes and classify how 
 * interesting are individual nodes.
 * 
 */

include_once 'meme.inc';
include_once 'content.inc';
include_once './'. drupal_get_path('module', 'machinelearningapi') .
  '/naive_bayes.inc';


/**
 * class machine_learning_api
 */
interface machine_learning_api {

  public function __construct($mid);
  
  /** Aggregations: */

  /** Compositions: */

  /**
   *
   * @return float
   * @access public
   */
  public function how_interesting($content);

  /**
   *
   * @param content content_array Array of content

   * @return meme
   * @access public
   */
  public function get_memes($content_array, $distance_threshold, $num_days);

  /**
   *
   * @param meme meme 
   * @return float
   * @access public
   */
  public function how_interesting_meme($meme);
  
  public function train_on_click_data($content_array, $timestamp);
} // end of machine_learning_api






/**
 * class simple_classifier1
 */
class simple_classifier1 implements machine_learning_api {

  public function __construct($mid) {
    $this->mid = $mid;
  }
  
  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/
  private $naive_bayes;
  
  
  /*
   * Singleton pattern to return naive bayes instance
   */
  public function get_naive_bayes_instance() {
    if (is_null($this->naive_bayes)) {
      $this->naive_bayes = new naive_bayes();
      return $this->naive_bayes;
    }
    else {
      return $this->naive_bayes;
    }
  }
  
  /**
   * Returns an interestingness score for a single content object
   *
   * @return float
   * @access public
   */
  public function how_interesting($content) {
    $nb = $this->get_naive_bayes_instance();
    $score = $nb->classify($content->get_content());
    return $score;
  }

  /**
   * Calculates memes from an array of content objects.
   *
   * Meme creation steps
   * 1. Calculate distance scores using MySQL fulltext search.
   * 2. Normalize distance scores to 0-1.
   * 3. Find interlinking between content
   * 4. Calculate clusters using Pycluster - an open source clustering library
   * written in C.
   * 5. Convert clusters into meme objects.
   * 6. Calculate interestingness of memes.
   * 7. Sort memes by interestingness and return.
   * 
   * @return array of memes
   * @access public
   */
  public function get_memes($content_array, $distance_threshold, $num_days) {
    $cutoff_point = time() - 86400 * $num_days;
    if (empty($content_array)) {
      return false;
    }

    $this->calc_distance_scores($content_array, $cutoff_point);
    
    // Find links and store in content objects
#    $this->find_links($content_array); // not written yet

    // Get array of clusters. Clusters are not yet memes. Headlines, discussions,
    // related content still needs to be determined.
    $clusters = $this->get_clusters($distance_threshold);
    
    // Convert array of clusters into meme objects.
    $memes = $this->get_memes_from_clusters($clusters, $content_array);
    
    // If there are no memes, return.
    if (empty($memes)) {
      return False;
    }
     // Sort memes
    usort($memes, array(&$this, "sort_memes_by_interestingness"));
      
    return $memes;
  }
  
  /*
   * Returns distance matrix for a content_array
   *
   * Loop through each content object and calculate using MySQL how "close"
   * different content is to each other. This score converted to a 
   * distance measure by the normalize_distances method to be used to find 
   * clusters of content.
   *
   *  NODES     1   2   3
   *        1 [0.0,1.2,4.4]
   *        2 [1.2,0.0,0.5] -- Distance matrix between nodes
   *        3 [4.4,0.5,0.0] -- Each column is calculated in the sql query below
   */
  private function calc_distance_scores($content_array, $cutoff_point) {

    $relevance_score_matrix = array();
    
    // Make a string of all CIDs in the content_array
    $this->cids = array();
    foreach ($content_array as $content) {
      $this->cids[] = $content->get_cid();
    }
#    dpm(count($this->cids) .' content objects');
    variable_set('ml_start', time());
    
    variable_set('start_inserting', time());
    foreach ($content_array as $content) {
      // Find what distance scores are already in the database. We don't need
      // to recalculate those
      $results1 = db_query("SELECT cid1 FROM 
      {machinelearningapi_distance_cache} WHERE cid2 = %d", $content->get_cid());
      $results2 = db_query("SELECT cid2 FROM 
      {machinelearningapi_distance_cache} WHERE cid1 = %d", $content->get_cid());
      
      $cids = array();
      while ($data = db_fetch_array($results1)) {
        $cids[] = $data['cid1'];
      }
      while ($data = db_fetch_array($results2)) {
        $cids[] = $data['cid2'];
      }
      $new_cids = array_diff($this->cids, $cids);
      
      // If there are no new cids, then continue
      if (count($new_cids) == 0) {
        continue;
      }

      $new_cids_str = array_to_sql_string($new_cids);
  
      // Use MySQL full text search to find distance score      
      $results = db_query("select cid, match(content)
      against('%s' ". /* WITH QUERY expansion */ ") as score from 
      {memetracker_search_%d} WHERE cid in ($new_cids_str)", 
      $content->get_content(), $this->mid);
      
      $scores = array();
      while ($data = db_fetch_array($results)) {
        $scores[$data['cid']] = $data['score'];
      }
      if (count($scores) == 0) {
        continue;
      }
      
      // insert scores into database
      foreach ($scores as $cid => $score) {
        if ($content->get_cid() < $cid) {
          $cid1 = $content->get_cid();
          $cid2 = $cid;
        }
        else {
          $cid1 = $cid;
          $cid2 = $content->get_cid();
        }
        $prev_value = db_result(db_query("SELECT * from 
        {machinelearningapi_distance_cache} WHERE cid1 = %d AND cid2 = %d",
        $cid1, $cid2));
        
        if (!$prev_value) {
          db_query("INSERT INTO {machinelearningapi_distance_cache} VALUES (
          %d, %d, %f)", $cid1, $cid2, $score);
        }
      }

    }
#      dpm(time() - variable_get('start_inserting', Null) .' seconds to calculate and insert new variables');  
  }
  
  private function normalize_distances($distance_matrix) {
    // For each distance subtract the smallest value divide by the largest distance.
    // Then subtract each by 1 and take absolute value to convert relevance score to distance 
    foreach ($distance_matrix as &$scores) {
      foreach ($scores as &$score) { // & assigns reference not copy
        $score = $score / 40;
        $score = abs($score - 1);
        if ($score > 1) {
          $score = 0;
        }
      }
    }
    // Last $score is not automatically destroyed per http://us.php.net/foreach
    unset($score);
    
    return $distance_matrix;
  }
  
  /*
   * Passes distance scores to cluster.py which uses Pycluster to calculate
   * clusters.
   * 
   * @return array of clusters in format [contentID, contentID, distance_score].
   */
  public function get_clusters($distance_threshold) {
    variable_set('start_clustering', time());
    $distance_matrix = array();
#    dpm(count($distance_matrix));

    $this->cids_str = array_to_sql_string($this->cids);
      
    foreach ($this->cids as $cid) {
      $results1 = db_query("SELECT cid1, search_score FROM 
      {machinelearningapi_distance_cache} WHERE cid1 in ($this->cids_str)
      AND cid2 = %d", $cid);
      $results2 = db_query("SELECT cid2, search_score FROM 
      {machinelearningapi_distance_cache} WHERE cid2 in ($this->cids_str)
      AND cid1 = %d", $cid);
      
      $scores = array();
      while ($data = db_fetch_array($results1)) {
        $scores[$data['cid1']] = $data['search_score'];
      }
      while ($data = db_fetch_array($results2)) {
        $scores[$data['cid2']] = $data['search_score'];
      }
      
      // Ensure all distance score arrays are aligned correctly
      ksort($scores);

      $distance_matrix[] = $scores;
    }
    
      // Convert relevancy scores from mysql fulltext search to distance scores
      // that the cluster library expects
      $distance_matrix = $this->normalize_distances($distance_matrix);
      
    // Prepare distance_matrix so can be read as string into the python script.
    $distance_data = "";
    foreach ($distance_matrix as $distance_col) {
      $distance_data .= ' || ';
      foreach ($distance_col as $distance) {
          $distance_data .= $distance .",";
      }
    }
    
    // Get directory for default 'files' directory
    $files_dir = file_directory_path();
    
    // Save data
    $file_path = realpath(file_save_data($distance_data, $files_dir .'/memetracker_data.txt', 
    FILE_EXISTS_REPLACE));
    
    $script_path = realpath(drupal_get_path('module', 'memetracker') .'/cluster.py');
    
    // call python script
    exec("python $script_path $file_path $distance_threshold", $clusters);

    // parse response and form into clusters
    $clusters = $clusters[0];

    // If no clusters are returned, return false
    if (!$clusters) {
      return False;
    }
    
    $clusters = trim($clusters, ';');
    $clusters = explode(';', $clusters);
    
    foreach ($clusters as &$cluster) {
      $cluster = explode(',', $cluster);
    }
    unset($cluster);
    
#    dpm(time() - variable_get('start_clustering', Null) .' seconds to cluster');
#    dpm('Number of clusters: '. count($clusters));
#    dpm($clusters);
    
    return $clusters;
    
  }
  /*
   * Converts array of clusters to meme objects.
   * 
   * @return array of memes
   */
  public function get_memes_from_clusters($clusters, $content_array) {
    variable_set('combine_memes', time());
    if (empty($content_array) && !$clusters) {
      return False;
    }
    
    $memes = array();
    // Record which content are in memes. Those content that aren't part
    // of a meme will have a meme created just for them
    $content_in_meme = array();
    
    // memes which are combined. They'll be unset at the end
    $merged_memes = array(); 

    // Identify headline -- for now, whatever content has the highest interestingness score
    if ($clusters) { // if clusters is not empty
      foreach ($clusters as $cluster) {
        $meme = new meme();
        
        // If the node values are negative, these variables point to a previously
        // created meme
        $left_meme_pointer = abs($cluster[0]) - 1;
        $right_meme_pointer = abs($cluster[1]) - 1;
        
        // If the node value are positive, these variables point to the content
        // within the $content_array
        $left_content_pointer = $cluster[0];
        $right_content_pointer = $cluster[1];
        
        // Set what is the sign of the nodes in the cluster 
        $left_is_neg = False;
        $right_is_neg = False;
        if ($cluster[0] < 0) {
          $left_is_neg = True;
        }
        if ($cluster[1] < 0) {
          $right_is_neg = True;
        }
        
        // If both the right and left nodes are positive
        if (!$left_is_neg && !$right_is_neg) {
          $meme->add_related_content($content_array[$left_content_pointer]);
          $meme->add_related_content($content_array[$right_content_pointer]);
          $content_in_meme[] = $left_content_pointer;
          $content_in_meme[] = $right_content_pointer;
        }
        
        // If both the right and left nodes are negative
        if ($left_is_neg && $right_is_neg) {
          $left_meme = $memes[$left_meme_pointer];
          $right_meme = $memes[$right_meme_pointer];

          $meme = $this->merge_memes($left_meme, $right_meme);

          // The two memes pointed from the left and right nodes of the cluster
          // are not needed any longer. $merged_memes stores their place in the array
          // to garbage collect later.
          $merged_memes[] = $left_meme_pointer;
          $merged_memes[] = $right_meme_pointer;
        }
        // If the left node is negative but the right node is positive
        if ($left_is_neg && !$right_is_neg) {
          // load already created meme that the left node points to
          $old_meme = $memes[$left_meme_pointer];
          
          // Add the right node which is content for the new meme
          $meme->add_related_content($content_array[$right_content_pointer]);
          
          $content_in_meme[] = $right_content_pointer;
                  
          // Add content from the already existing meme to the new meme
          $meme->add_related_content($old_meme->get_related_array());
          
          // Because the content from the old meme was merged into the new meme, 
          // the old meme needs to be garbage collected. 
          // $merged_memes stores which memes need garbage collected later
          $merged_memes[] = $left_meme_pointer;
        }
        
        // If the left node is positive but the right node is negative
        if (!$left_is_neg && $right_is_neg) {
          // load already created meme that the right node points to
          $old_meme = $memes[$right_meme_pointer];
                  
          // Add the left node which is content to the new meme
          $meme->add_related_content($content_array[$left_content_pointer]);
          
          $content_in_meme[] = $left_content_pointer;
          
          // Add content from the already exisiting meme to the new meme
          $meme->add_related_content($old_meme->get_related_array());
          
          $merged_memes[] = $right_meme_pointer;
        }
        // Add new meme to $memes array
        $memes[] = $meme;
      }
    }
    
    // Set memes to be null that were later combined from memes array
    foreach ($merged_memes as $meme_id) {
      $memes[$meme_id] = Null;
    }
    
    // This two step process is necessary so the meme_ids stay lined up with the 
    // order of memes in the $memes array
    $count = count($memes);
    for ($i = 0; $i < $count; $i++) {
      if (is_null($memes[$i])) {
        unset($memes[$i]);
      }
    }

    // Create clusters for each content that didn't make it into a "real" cluster
    for ($i = 0; $i < count($content_array); $i++) {
      if (!in_array($i, $content_in_meme)) {
        $meme = new meme();
        $meme->add_related_content($content_array[$i]);
        $memes[] = $meme;
      }
    }

    foreach ($memes as &$meme) {
      $this->calculate_headline($meme);
#      $this->remove_multiple_articles_same_source($meme); // not working yet
      $this->how_interesting_meme($meme);
    }
    unset($meme);

#    dpm(time() - variable_get('combine_memes', Null) .' seconds to combine memes');
#    dpm((time() - variable_get('ml_start', Null)) .' seconds total');
    return $memes;
  }
  
  /*
   * Calculate which content is the headline
   * Eventually this will be a very complex process, for now just pick the 
   * content with the highest interestingness score
   */
  function calculate_headline($meme) {
    $related_array = $meme->get_related_array();

    // Take a short cut if only one related content.
    if (count($related_array) == 1) {
      $meme->set_headline($related_array[0]);
      $meme->set_related_array(Null);
      return;
    }
    
    $most_interesting = 1;
    $headline = 0;
    for ($i = 0; $i < count($related_array); $i++) {

      if ($related_array[$i]->get_interestingness() < $most_interesting) {
        $most_interesting = $related_array[$i]->get_interestingness();
        $headline = $i;
      }
    }
    $meme->set_headline($related_array[$headline]);
    unset($related_array[$headline]);

    $meme->set_related_array($related_array);
  }
  
  /*
   * Helper function which merges two memes
   * @return meme
   */
  function merge_memes($meme_1, $meme_2) {
    $meme = new meme();
    $related_array = array_merge($meme_1->get_related_array(), $meme_2->get_related_array());
    $meme->set_related_array($related_array);

    return $meme;
  }
  
  /*
   * Each meme should have a maximum of two content per source. This function
   * removes extras.
   */
  function remove_multiple_articles_same_source($meme) {
    if (count($meme->get_related_array()) > 1) {  
      // add articles to array ['source'] => array(article1, article2, etc);
      $array = array();
      $related_array = $meme->get_related_array();
      $rel_array = array();
      foreach ($related_array as $related_content) {
        $array[$related_content->get_source_name()][] = $related_content;
      }
#      if (count($array) > 2) {dpm($array);}
      // Then loop through, if there's more than two content in a source, 
      // lop off articles with lowest interestingness.
      foreach ($array as $source => &$content) {
        if (count($content) > 1) {
          $max = $content[0];
          foreach ($content as $c) {
            if ($c->get_interestingness() > $max->get_interestingness()) {
              $max = $c;
            }
          }
          $content = array($max);
          foreach ($array as $source => $content) {
            $rel_array[] = $content[0];
          }
        }
      }
    }
    if (($rel_array)) {
      $meme->set_related_array($rel_array);
    }
  }
  
  /*
   * Sorting function. Returns meme with a higher interestingness score
   */
    static function sort_memes_by_interestingness($a, $b) {
    if ($a->get_interestingness() < $b->get_interestingness()) {
      return 1;
    }
    else {
      return -1;
    }
  }
  /**
   * Returns interestingness score for an entire meme - this will soon be a hook.
   * @param meme meme 
   * @return float
   * @access public
   */
  public function how_interesting_meme($meme) {
    $related_array = $meme->get_related_array();
    
    $avg_time = 0;
    $count = 0;
    
    // Find the average time of creation of all the content in the memes.
    if (!empty($related_array)) {
      foreach ($related_array as $content) {
        $count++;
        $avg_time += $content->get_timestamp();
  //      loop through discussions here as well
      }
    }
    $avg_time += $meme->get_headline()->get_timestamp();
    $count++;
    
    // Find difference from now until the average creation time of the content
    // in the meme in hours.
    $avg_time = (time() - ($avg_time / $count)) / 360;
    
    // This algorithm is roughly based on Hacker News' algorithm found here:
    // http://www.seomoz.org/blog/reddit-stumbleupon-delicious-and-hacker-news-algorithms-exposed
    $interestingness = pow($count, 3) / pow($avg_time + 4, 1.1);
    
    $meme->set_interestingness($interestingness);
    
    $this->i[] = $interestingness;
    $this->cou[] = $count;
    
    return $meme;
  }
  
  /*
  * Trains naive_bayes using content that received a click.
  */
  public function train_on_click_data($content_array, $timestamp) {
     // just train naive_bayes for now -- needs to add adjustment for number of clicks, multiplier of some sort?
    $naive_bayes = $this->get_naive_bayes_instance(); //naive_bayes()
    foreach ($content_array as $content) {
      if ($content->get_click_data($timestamp) > 0) {
        $naive_bayes->train($content->get_content(), 'interesting');
      }
    }
  }
  
  /*
   * Takes array of content objects and sets interestingness score for each object
   * if a score hasn't been set yet.
   */
  function calculate_content_interestingness($content_array) {
    foreach ($content_array as $content) {
      // These content haven't been given an interestingness score yet.
      if ($content->get_interestingness() == 0) {
        $content->set_interestingness($this->how_interesting($content));
      }
    }
  }
}

<?php
// $Id: naive_bayes.inc,v 1.2 2008/08/11 22:16:57 kylemathews Exp $

/**
 * @file
 * Implmentation of naive_bayes algorithm.
 * http://en.wikipedia.org/wiki/Naive_Bayes_classifier
 * 
 */

/*
 * class naive_bayes
 */
class naive_bayes {
  function naive_bayes_classify_node($nid) {
    $node = node_load($nid);
    $text = $node->title .' '. $node->body;

    $score = classify($text);
    print drupal_to_js($score);
    return;
  }

   // naive bayes stuff
  // need two tables - nb_fc and nb_cc
  // break document up into words
  function get_words($text) {
    // make words lowercase
    $text = strtolower($text);
      // Strip html tags
    $text = strip_tags($text);
    $features = preg_split("/\W+/", $text);
    
    return $features;
  }
  // Increase the count of a feature/category pair
  function incf($f, $cat) {
    $count=$this->fcount($f, $cat);
    if ($count==0) {
      db_query("INSERT INTO {naive_bayes_fc} VALUES ('%s', '%s', 1)", $f, $cat);
    }
    else {
      db_query("UPDATE {naive_bayes_fc} SET count = %d WHERE feature='%s' and
      category = '%s'", $count + 1, $f, $cat);
    }
  }
  // Increase the count of a category
  function incc($cat) {
    $count = $this->catcount($cat);
    if ($count==0) {
      db_query("INSERT INTO {naive_bayes_cc} VALUES ('%s', 1)", $cat);
    }
    else {
      db_query("UPDATE {naive_bayes_cc} SET count = %d WHERE category = '%s'",
      $count + 1, $cat);
    }
  }
  // The number of times a feature has appeared in a category
  function fcount($f, $cat) {
    $result = db_result(db_query("SELECT count FROM {naive_bayes_fc} WHERE feature = '%s'
    AND category = '%s'", $f, $cat));
    if ($result == null) {
      return 0;
    }
    else {
      return $result;
    }
  }
  // The number of items in a category
  function catcount($cat) {
    $result = db_result(db_query("SELECT count FROM {naive_bayes_cc} WHERE category
    = '%s'", $cat));
    
    return $result;
  }

  // The total number of items
  function totalcount() {
    $result = db_result(db_query("SELECT sum(count) FROM {naive_bayes_cc}"));
    if (!$result) {
      return 0;
    }
    else {
      return $result;
    }
  }
  // The list of all categories
  function categories() {
    $cats = array();
    $result = db_query("SELECT category FROM {naive_bayes_cc}");
    while ($cat = db_fetch_object($result)) {
      $cats[] = $cat->category;
    }
    return $cats;
  }
  // The total number of times these feature has appeared in this category
  // divided by the total number of items in this category
  function fprob($f, $cat) {
    if ($this->catcount($cat) == 0) {
      return 0;
    }

    return ($this->fcount($f, $cat) / $this->catcount($cat));
  }
  
  // Fisher probabilty
  function cprob($f, $cat) {
    // The frequency of this feature in this category
    $clf = $this->fprob($f, $cat);

    if ($clf == 0) {
      return 0;
    }
    return $clf;
    
    // There isn't any other category for now other than interestingness
#    // The frequency of this feature in all the categories
#    $freqsum = 0;
#    foreach ($this->categories() as $c) {
#      $freqsum += $this->fprob($f, $c);
#    }
#    
#    // The probability is the frequency in this category divided by the overall frequency
#    $p = $clf / $freqsum;
#    return $p;
  }

  // Weighted probability returns better results when there's few features in a 
  // category
  function weighted_prob($f, $cat) {
    $weight = 5.0;
    $ap = 0.5;
    
    // Calculate current probabilty
    $basicprob = $this->cprob($f, $cat);
#    dpm('basicprob from cprob(): '. $basicprob);
    // Count the number of times this feature has appeared in all categories
    $totals = 0;
    $categories = $this->categories();
    
    // count number of times 
    foreach ($categories as $category) {
  #    dpm($f);
  #    dpm($category);
      $count = $this->fcount($f, $category);
#      dpm($count);
      $totals += $count;
    }
#    dpm('totals: '. $totals);
    // Calculate the weighted average
    $bp = (($weight * $ap) + ($totals * $basicprob)) / ($weight + $totals);
    return $bp;
  }
  // Classify nodes using Fisher Method
  // returns probability document fits a category
  function classify_cat($text, $cat) {
    // Multiply all the probabilities together
    $p = 1;
    $features = $this->get_words($text);
    foreach ($features as $feature) {
#      dpm('feature: '. $feature);
      $p *= $this->weighted_prob($feature, $cat);
    }
#    dpm($p);
    // Take the natural log and multiply by -2
    $fscore = -2 * log($p);
    
    // Use the inverse chi2 function to get a probability
    return $this->invchi2($fscore, sizeof($features) * 2);
  }
  function invchi2($chi, $df) {
    $m = $chi / 2.0;
    $sum = exp(-$m);
    $term = $sum;
    for ($i = 1; $i < $df/2; $i++) {
      $term *= $m / $i;
      $sum += $term;
    }
    return min($sum, 1.0);
  }

  // API type stuff
  // Train naive bayes with node and if node is good or bad
  function train($text, $cat) {
    // Split words into array
    $features = $this->get_words($text);
    foreach ($features as $feature) {
      // Increment the count for every feature in this category
      $this->incf($feature, $cat);
      
      // Increment the count for this category
      $this->incc($cat);
    }
  }
  function classify($text) {
    // Get probability for both good and bad category
    $good = $this->classify_cat($text, "interesting");
#    $bad = $this->classify_cat($text, "uninteresting");
  #  dpm($good);
  #  dpm($bad);
#    return ($good - $bad);

    return $good;
  }
}

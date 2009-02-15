<?php
// $Id: meme.inc,v 1.1 2008/06/27 17:52:58 kylemathews Exp $

/**
 * @file meme.inc
 * Defines the meme object
 *
 */

require_once 'content.inc';


/**
 * class meme
 */
class meme {

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/

  /**
   * @access private
   */
  public $headline;

  /**
   * @access private
   */
  public $discussion_array;

  /**
   * @access private
   */
  public $related_array;

  /**
   * @access private
   */
  public $interestingness;

  public function get_headline() {
    return $this->headline;
  }
  
  public function set_headline($headline) {
    $this->headline = $headline;
  }

  public function get_discussion_array() {
    return $this->discussion_array;
  }
  
  public function set_discussion_array($discussion_array) {
    $this->discussion_array = $discussion_array;
  }
  
  public function get_related_array() {
    return $this->related_array;
  }
  
  public function set_related_array($related_array) {
    $this->related_array = $related_array;
  }
  
  public function get_interestingness() {
    return $this->interestingness;
  }
  
  public function set_interestingness($interestingness) {
    $this->interestingness = $interestingness;  
  }
  
  // accepts both an array or individual piece of content
  public function add_related_content($content) {
    if (is_null($content)) {
      return;
    }
    else {
      if (is_array($content)) {
        foreach ($content as $c) {
          if (!is_null($c)) {
            $this->related_array[] = $c;
          }
        }
      }
      else {
        $this->related_array[] = $content;
      }
    }
  }

  public function add_discussion_content($content) {
    $this->discussion_array[] = $content;
  }
} // end of meme

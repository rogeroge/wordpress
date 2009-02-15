<?php
// $Id: meme.tpl.php,v 1.2 2008/08/30 13:11:17 kylemathews Exp $

/**
 * @file meme.tpl.php
 * Default theme implementation to render a meme
 *
 * Variables available:
 *   $meme - a single meme object
 *   $id - by order meme falls, 0 based
 *
 */
 
drupal_add_js(drupal_get_path('module', 'memetracker') . '/meme.js');
drupal_add_css(drupal_get_path('module', 'memetracker') . '/meme.css'); 

?>

<?php 
if ($meme) {
  $content = $meme->get_headline(); 
  $related_array = $meme->get_related_array();
  }
?>

<!--  <div class="score"> Score: <?php // print $meme->get_interestingness() ?> </div> -->
  <div class="meme">
  <?php print "<div class = \"content_source\">". 
  $content->get_source_link() ."</div>" ?>  
  <h2> 
  <?php print l($content->get_title(), $content->get_link(), 
  array("attributes" => 'mid', "query" => 'mid=' . $content->get_mid(),
  "fragment" => 'cid='. $content->get_cid())) 
  ?>
  </h2>

  <?php print "<p>" . strip_tags(node_teaser($content->get_body(), Null, 400)) . "</p>"; ?>

<!-- Begin minimized discussion section -->
  <?php if (!empty($related_array)) {
      print '<div class="toggle"><span class = '. $id .'> + </span></div>';
      print '<div class="discussion discussion-min '. $id .'">';
      print '<span>Related:</span> ';
      $discussion;
      foreach ($related_array as $related_content) {
        $discussion .= l($related_content->get_source_name(), $related_content->get_link(), 
          array("query" => 
          'mid=' . $related_content->get_mid(), 
          "fragment" => 'cid='. $related_content->get_cid()));
        $discussion .= ', ';
      }
      $discussion = rtrim($discussion, ', ');
      print $discussion;
      
      print '</div><br />'; // End discussion
    }
  ?>
<!-- End minimized discussion section -->


<!-- Begin maximized discussion section -->
  <?php if (!empty($related_array)) {
      print '<div class="discussion discussion-max '. $id .'">';
      print '<span>Related:</span> ';
      $discussion = '<br />';
      $discussion .= '<div class="discussion-block">';
      foreach ($related_array as $related_content) {
        $discussion .= '<span class="discussion-title>"';
        $discussion .= $related_content->get_source_link() .':</span> ';
        $discussion .= l($related_content->get_title(), $related_content->get_link(), 
          array("query" => 
          'mid=' . $related_content->get_mid(), 
          "fragment" => 'cid='. $related_content->get_cid()));
        $discussion .= '<br /> ';
      }
      $discussion = rtrim($discussion, ', ');
      $discussion .= '</div>';
      print $discussion;
      
      print '</div>'; // End discussion
    }
  ?>
<!-- End maximized discussion section -->
</div> <!-- End Meme Div -->
<br /><br />


<!-- Begin related content -->
  <!-- <?php if (!empty($related_array)) {
    /*
    print '<div class="related_content">';
    print "<h3>Related:</h3>";
    foreach ($related_array as $related_content) {
      print "<div class = \"content_source\">". $related_content->get_source() ."</div>";    
      print "<h2>";
      print l($related_content->get_title(), $related_content->get_link(), 
        array("attributes" => 'mid', "query" => 'mid=' . $related_content->get_mid(), 
        "fragment" => 'cid='. $related_content->get_cid()));
      print "</h2>";
      print "<p>" . strip_tags(node_teaser($related_content->get_body(), Null, 400)) . "</p>";
#      print "<br /> ";
    }
    print '</div>';
  }
*/
}

  ?> -->
<!-- End related content -->

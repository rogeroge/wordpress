<?php
// $Id: memebrowser.tpl.php,v 1.11 2008/08/30 13:11:17 kylemathews Exp $

/**
 * @file memebrowser.tpl.php
 * Default theme implementation to display a memebrowsing page
 *
 * Variables available:
 *   $memes - array of meme objects
 *
 */
drupal_add_js(drupal_get_path('module', 'memetracker') . '/memetracker.js');
drupal_add_css(drupal_get_path('module', 'memetracker') . '/memebrowser.css');
global $base_url;
// Pass base_url as variable to page.
drupal_add_js("var base_url = \"" . $base_url . "\";", 'inline'); 

// TODO convert this so loops through memes and calls theme('meme', $meme);
// then the meme would call theme('headline_meme', $headline) and theme for 
// related content etc.
?>

<?php 
  if ($memes) {
    // Loop through memes calling meme.tpl.php to render each meme.
    $id = 0;
    foreach ($memes as $meme) {
      print theme('meme', $meme, $id);
      $id++;
    }
  }
  else {
    // this help should be stored somewhere else eventually. . . now where. . .?
      print "<p>Sorry, there are no memes to display (and what's a memetracker without a 
    meme?)!</p >";
    print "<p>There is a few reasons why memes might not be showing. If you 
    haven't finished the installation steps found in INSTALL.txt, please follow
    finish the installation steps which are reproduced below. The next thing to 
    try is to ". l('run cron',
    '/admin/reports/status/run-cron?destination=memetracker/1') .". 
    Only content from the past two days is shown. 
    If you haven't run cron for more than two days, there is no new content 
    to show. Running cron will download new content for display.";
    print "<p>If you've just enabled both the Memetracker and MachinelearningAPI
    modules, please follow the remaining installation steps below taken from the
     INSTALL.txt found in the memetracker directory:</p>";
    
    print "<blockquote>";
    
    $file_path = drupal_get_path('module', 'memetracker') .'/INSTALL.txt';

    $fh = fopen($file_path, 'r') or die("Can't open file");

    // Print the Install file
    $text = "";
    while ($line = fgets($fh)) {
      $text .= '<p>'. $line .'</p>';
    }
    fclose($fh);
    print _filter_url($text, -1);

    print "</blockquote><br />";
    
    print "<p>To report bugs and request support, please post an issue at:
    <a href=\"http://drupal.org/project/issues/memetracker\">
    http://drupal.org/project/issues/memetracker</a></p>";
    }

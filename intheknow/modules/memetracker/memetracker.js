// $Id: memetracker.js,v 1.6 2008/08/30 13:11:17 kylemathews Exp $

// Global killswitch: only run if we are in a supported browser.
if (Drupal.jsEnabled) {
  $(document).ready(
    function(){
      $('.meme a:not(".toggle span"), .meme h2 a').click(function(){
        var re = new RegExp("/?mid=([0-9]+).cid=([0-9]+)");
/*        alert(re);*/
        var matches = this.href.match(re);
/*        alert(" mid: " + matches[1] + " cid: " + matches[2]);*/
        $.post(base_url + "/memetracker_click/" + matches[1] + '/' + matches[2]);
/*        return false;*/
      });
    }
  );
}

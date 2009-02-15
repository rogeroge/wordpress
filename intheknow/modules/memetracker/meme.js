// $Id: meme.js,v 1.2 2008/08/30 13:11:17 kylemathews Exp $
// Global killswitch: only run if we are in a supported browser.
if (Drupal.jsEnabled) {
  // Set event handler. Listen for clicks and toggle between minimized and
  // maximized versions of discussions.
  $(document).ready(
    function(){
      $(".toggle span").mouseup(function(){
        var class = $(this).attr("class");
        
        $(".discussion-min."+class).toggle();
        $(".discussion-max."+class).toggle();
        
        $(".discussion-min."+class).each(function(){
          if ($(this).is(":hidden")) {
            $(".toggle span."+class).html("-").css({ "padding-left":"5px", 
            "padding-right":"5px" });
          }
          else {
            $(".toggle span."+class).html("+").css({ "padding-left":"2px", 
            "padding-right":"2px" });;
          }
        });

        
        return false;
      });
    }
  );
  
  $(document).ready(
    function(){
      $(".toggle").hover(
        function () {
          $(this).addClass("yellow_glow");
        }, 
        function () {
          $(this).removeClass("yellow_glow", "slow");
        }
      );
    }
  );
}

<?php
reason_include_once( 'minisite_templates/modules/default.php' );
$GLOBALS[ '_module_class_names' ][ basename( __FILE__, '.php' ) ] = 'MobileIconHomeModule';

class MobileIconHomeModule extends DefaultMinisiteModule {
    function init( $args = array() ) {

    }

    function has_content() {
        return true;
    }

    function run() {
        echo '<div class="center">';
        echo '<ol class="icon-menu">';
        //echo '<li id="menu-cafcam">';
        //echo '<a accesskey="3" href="https://reasondev.luther.edu/mobile/comingsoon/">Caf Cam</a>';
        //echo '</li>';
        echo '<li id="menu-map">';
        echo '<a accesskey="3" href="map/">Campus Map</a>';
        echo '</li>';
        echo '<li id="menu-directions">';
        echo '<a accesskey="4" href="directions/">Directions</a>';
        echo '</li>';
        echo '<li id="menu-directory">';
        echo '<a accesskey="5" href="https://www.luther.edu/directory/">Directory</a>';
        echo '</li>';
        //echo '<li id="menu-find">';
        //echo '<a accesskey="7" href="https://find.luther.edu/">Find</a>';
        //echo '</li>';
        echo '<li id="menu-labstat">';
        echo '<a accesskey="6" href="labs/">Lab Availability</a>';
        echo '</li>';
        echo '<li id="menu-library_search">';
        echo '<a accesskey="7" href="librarysearch/">Library Search</a>';
        echo '</li>';
        echo '<li id="menu-news">';
        echo '<a accesskey="8" href="news/">News</a>';
        echo '</li>';
        echo '<li id="menu-mail">';
        echo '<a accesskey="9" href="http://mail.luther.edu/">Norse Mail</a>';
        echo '</li>';
        echo '<li id="menu-home">';
        echo '<a accesskey="2" href="http://www.luther.edu/">Full Site</a>';
        echo '</li>';
        echo '</ol>';
        echo '</div>';
    }
}
?>

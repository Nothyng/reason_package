<?php
	reason_include_once( 'minisite_templates/modules/default.php' );
	
	$GLOBALS[ '_module_class_names' ][ 'global/'.basename( __FILE__, '.php' ) ] = 'GlobalHeader';
	
	class GlobalHeader extends DefaultMinisiteModule
	{
		function init( $args = array() )
		{
		}
		
		function has_content()
		{
			return true;
		}

		function home_url()
		{
			return luther_is_sports_page() ? '<a href="/sports"><span>Luther College Sports</span></a>' : '<a href="/"><span>Luther College</span></a>';
		}
		
		function run()
		{
		
		?>
			<header id="global-banner" role="banner">
			
				<hgroup>
					<h1><?php echo $this->home_url() ?></h1>
					<h2><span>Global Site Tagline</span></h2>
				</hgroup>
				
				<div id="mobile-nav" role="navigation">
					<ul>
						<li>
							<a class="toggle left-off-canvas-toggle" href="#global-nav">
								<i class="fa fa-bars"></i>
								<span>Global Navigation</span>
							</a>
						</li>
						<li>
							<a class="toggle search" href="#search-nav">
								<i class="fa fa-search"></i>
								<i class="fa fa-times"></i>
								<span>Search Navigation</span>
							</a>
						</li>
					</ul>
				</div>
			
				<div id="search-nav" role="navigation">
					<ul>
						<li class="directory"><a href="/directory/">Directory</a></li>
						<li class="offices"><a href="/contact/administrative">Offices</a></li>
						<li class="index"><a href="/azindex/">A-Z Index</a></li>
						<li class="search">
							<gcse:searchbox-only></gcse:searchbox-only>
						</li>
					</ul>
				</div>
			
			</header>	
			
			<?php
		}
	}
?>
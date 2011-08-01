<?php

/**
 * Functions for manipulating images.
 * @package carl_util
 * @subpackage basic
 * @author Eric Naeseth <enaeseth+reason@gmail.com>
 */

require_once 'filesystem.php';
require_once 'misc.php';

/**
 * Scale an image to meet a maximum width and height.
 * 
 * The image will be resized in place; to avoid this, make a {@link copy} of
 * the original image first, and resize the copy. 
 * 
 * Note that the image will maintain its aspect ratio and be fitted to a box of the given width and height.
 *
 * @todo check whether gd_image is working properly 
 *
 * Raises a {@link WARNING} if no image resize method is available.
 * 
 * @param string $path filesystem path to the image to be resized
 * @param int $width desired maximum width
 * @param int $height desired maximum height
 * @param boolean $sharpen if true, the resized image will be sharpened
 * @return boolean true if the image was resized successfully; false if not
 */
function resize_image($path, $width, $height, $sharpen=true)
{
    if (!is_file($path) || !is_readable($path)) {
        trigger_error('cannot resize image; no file exists at the given path '.
            '('.var_export($path, true).')', WARNING);
        return false;
    }
    $perms = substr(sprintf('%o', fileperms($path)), -4);
    if (imagemagick_available()) {
        $result = _imagemagick_resize($path, $width, $height, $sharpen);
    } else if (function_exists('imagecreatetruecolor')) {
        $result = _gd_resize($path, $width, $height, $sharpen);
    } else {
	trigger_error('neither ImageMagick nor GD are available; cannot '.
            'resize image', WARNING);
        return false;
    }

    // Prevent the transformation from changing the file permissions.
    clearstatcache();
    $newperms = substr(sprintf('%o', fileperms($path)), -4);
    if ($perms != $newperms) @chmod($path, octdec($perms));
    return $result;
}

/**
* crop an image while preserving aspect ratio
*
* @param int $nw the new width of the image
* @param int $nh the new height of the image
* @param string $source the path and file of the image to be cropped
* @param string $dest the path and file of the cropped image
* @return boolean true on success
*
*/
function crop_image($nw, $nh, $source, $dest, $sharpen=true)
{
    if (!is_file($source) || !is_readable($source)) {
        trigger_error('cannot resize image; no file exists at the given path '.
            '('.var_export($source, true).')', WARNING);
        return false;
    }
    $perms = substr(sprintf('%o', fileperms($source)), -4);
   if (imagemagick_available()) {
        $result = _imagemagick_crop_image($nw, $nh, $source,$dest,$sharpen);
    } else if (function_exists('imagecreatetruecolor')) {
        $result = _gd_crop_image($nw, $nh, $source, $dest, $sharpen);
    } else {
	trigger_error('neither ImageMagick nor GD are available; cannot '.
            'crop image', WARNING);
        return false;
    }

    // Prevent the transformation from changing the file permissions.
    clearstatcache();
    $newperms = substr(sprintf('%o', fileperms($dest)), -4);
    if ($perms != $newperms) @chmod($dest, octdec($perms));
    return $result;
}

/**
* Crop the image using the Imagemagick library
* See crop_image for an explanation of the parameters
* and return value
*/
function _imagemagick_crop_image($nw,$nh,$source,$target,$sharpen)
{
	$info = getimagesize($source);
	$ow=$info[0];
	$oh=$info[1];
	
	$or=$ow/$oh;
	$nr=$nw/$nh;
	
	$x=0;
	$y=0;
	
//	pray($info);

	$resize_str="";
	if($nw<=$nh && $or>=$nr)
	{

		$resize_str=" -resize x".$nh." ";

	}
	elseif($nw<=$nh && $or<$nr)
	{
		$resize_str=" -resize ".$nw."x ";
	}
	elseif($nw>$nh && $or<$nr)
	{
		$resize_str=" -resize ".$nw."x ";
	}
	elseif($nw>$nh && $or>$nr)
	{
		$resize_str=" -resize x".$nh." ";;
	}
	else
	{
		$resize_str=" -resize ".$nw."x ";

	}
	$sharpen_str="";
	if($sharpen)
	{
		$sharpen_str=" -sharpen 1";
	}
	
	$repage=" -repage ".$nw."x".$nh;

	$exec="convert ".$resize_str.$sharpen_str."-gravity Center +repage  -crop ".$nw."x".$nh."+".$x."+".$y."! ".$source."  ".$target;

	$output = array();
	$exit_status = -1;
	exec($exec, $output, $exit_status);
	if ($exit_status != 0) {
		trigger_error('image crop failed: '.implode('; ', $output), WARNING);
		return false;
	}
	else
	{
		return true;
	}
}

/**
* Crop the image using the GD library
* See crop_image for an explanation of the parameters
* and return value
*/
function _gd_crop_image($nw, $nh, $source,$dest,$sharpen) 
{
	$details=_gd_get_image($source);
	$w=$details['width'];
	$h=$details['height'];
	$simg=$details['image'];
  	$dimg = imagecreatetruecolor($nw, $nh);

	$transindex = imagecolortransparent($simg);
	if($transindex >= 0) {
	  $transcol = imagecolorsforindex($simg, $transindex);
	  $transindex = imagecolorallocatealpha($dimg, $transcol['red'], $transcol['green'], $transcol['blue'], 127);
	  imagefill($dimg, 0, 0, $transindex);
	}


  	$wm = $w/$nw;
  	$hm = $h/$nh;
  	$h_height = $nh/2;
  	$w_height = $nw/2;

	$ow=$w;
	$oh=$h;
	$or=$ow/$oh;
	$nr=$nw/$nh;

	$success=true;
  	if($nw<=$nh && $ow>$oh) 
  	{
  		$adjusted_width = $w/ $hm;
  		$half_width = $adjusted_width / 2;
  		$int_width = $half_width - $w_height;
  		$success=imagecopyresampled($dimg,$simg,-$int_width,0,0,0,$adjusted_width,$nh,$w,$h);
  	}
	elseif($nw<=$nh && $ow<$oh && $or<=$nr)
	{
		$adjusted_height = $h/ $wm;
		$half_height = $adjusted_height / 2;
		$int_height = $half_height - $h_height;
		$success=imagecopyresampled($dimg,$simg,0,-$int_height,0,0,$nw,$adjusted_height,$w,$h);
	}
	elseif($nw<=$nh && $ow<$oh && $or>$nr)
  	{
  		$adjusted_width = $w/ $hm;
  		$half_width = $adjusted_width / 2;
  		$int_width = $half_width - $w_height;
  		$success=imagecopyresampled($dimg,$simg,-$int_width,0,0,0,$adjusted_width,$nh,$w,$h);
  	}
	elseif($nw>=$nh && $ow<$oh)
	{
		$adjusted_height = $h/ $wm;
		$half_height = $adjusted_height / 2;
		$int_height = $half_height - $h_height;
		$success=imagecopyresampled($dimg,$simg,0,-$int_height,0,0,$nw,$adjusted_height,$w,$h);
	}
	elseif($nw>$nh && $ow>=$oh && $or>=$nr)
	{
		$adjusted_height = $h/ $wm;
		$half_height = $adjusted_height / 2;
		$int_height = $half_height - $h_height;
		$success=imagecopyresampled($dimg,$simg,0,-$int_height,0,0,$nw,$adjusted_height,$w,$h);
	}
	elseif($nw>$nh && $ow>=$oh && $or<$nr)
	{
  		$adjusted_width = $w/ $hm;
  		$half_width = $adjusted_width / 2;
  		$int_width = $half_width - $w_height;
  		$success=imagecopyresampled($dimg,$simg,-$int_width,0,0,0,$adjusted_width,$nh,$w,$h);
	}
	else 
	{
		$success=imagecopyresampled($dimg,$simg,0,0,0,0,$nw,$nh,$w,$h);
	}
	if(!$success)
	{
		trigger_error("imagecopyresampled failed");
	}
	
	// Sharpen
	if ($sharpen) 
	{
	    $dimg = sharpen_image($image_dest, 80, 0.5, 3);
	}
	
	switch($stype)
	{
  		case 'gif':
  			$ret=imagegif($dimg,$dest,100);
  		break;
  		case 'jpg':
  			$ret=imagejpeg($dimg,$dest,100);
  		break;
  		case 'png':
  			$ret=imagepng($dimg,$dest,0);
  		break;
		
	}
	

    imagedestroy($dimg);
	imagedestroy($simg);
	return $ret;
}	


/*
function _gd_crop_image($nw, $nh, $source,$dest,$sharpen) 
{
  	$size = getimagesize($source);
  	$w = $size[0];
  	$h = $size[1];

	$stype="";
 	$type=strtolower(substr($source,-3));
	if($type=="gif" || $type=="jpg" || $type=="png")
	{
		$stype=$type;
	}
	else
	{
		trigger_error("$type is not a supported image type file name must have suffix 'gif', 'jpg' or 'png");
	}

  	switch($stype) 
	{
  		case 'gif':
  			$simg = imagecreatefromgif($source);
  		break;
  		case 'jpg':
  			$simg = imagecreatefromjpeg($source);
  		break;
  		case 'png':
  			$simg = imagecreatefrompng($source);
  		break;
  	}
  	$dimg = imagecreatetruecolor($nw, $nh);
  	$wm = $w/$nw;
  	$hm = $h/$nh;
  	$h_height = $nh/2;
  	$w_height = $nw/2;
  	if($w> $h) 
  	{
  		$adjusted_width = $w/ $hm;
  		$half_width = $adjusted_width / 2;
  		$int_width = $half_width - $w_height;
  		imagecopyresampled($dimg,$simg,-$int_width,0,0,0,$adjusted_width,$nh,$w,$h);
  	} 
	elseif(($w <$h) || ($w == $h)) 
	{
  		$adjusted_height = $h/ $wm;
  		$half_height = $adjusted_height / 2;
  		$int_height = $half_height - $h_height;
  		imagecopyresampled($dimg,$simg,0,-$int_height,0,0,$nw,$adjusted_height,$w,$h);
  	} 
	else 
	{
		imagecopyresampled($dimg,$simg,0,0,0,0,$nw,$nh,$w,$h);
	}
	// Sharpen
	if ($sharpen) 
	{
	    $dimg = sharpen_image($image_dest, 80, 0.5, 3);
	}
	
	switch($stype)
	{
  		case 'gif':
  			$ret=imagegif($dimg,$dest,100);
  		break;
  		case 'jpg':
  			$ret=imagejpeg($dimg,$dest,100);
  		break;
  		case 'png':
  			$ret=imagepng($dimg,$dest,100);
  		break;
		
	}
	

    imagedestroy($dimg);
	imagedestroy($simg);
	return $ret;
}	
*/

/**
* copies one image over another
* makes sure the option array is populated with default values if
* it has not been set
* determines if Imagemagick is install, and calls the _imagemagick_blit_image function
* if Imagemagick is not installed it looks to see if GD is available and
* calls _gd_blit_image if it is
* whimpers and cries if neither Imagemagick or GD are available.
*
* NOTE: paths are absolute paths on the server
*
* @param $source is the path and file of the image to
*        on the bottom, (The Background) (Note that $source
*        does not get rewritten in this operation.) 
* @param $dest is the path and file of the final result
*        after copying
* @param $watermarak is the path and file of the image on top
* @param $option gives the origin of the coordinate system to 
*        be used when blitting
*
*        $options['horizontal']= "left", "right", or "center"
*        $options['vertical']="top", "bottom" or "center"
*        $options['horizontal_offset]= number of pixels to move horizontally before beginning to blit
*        $options['vertical_offset']= number of pixels to move vertically before beginning to blit
*		 default values are ("left","top",0,0) if you do not set the options array
*
* @return returns true upon success and false otherwise. (There is no room for failure!)
*/
function blit_image($source,$dest,$watermark,$options=array())
{
	//process the options
	if(!isset($options['horizontal']))
	{
		$options['horizontal']="left";
	}
	if(!isset($options['horizontal_offset']))
	{
		$options['horizontal_offset']=0;
	}
	if(!isset($options['vertical']))
	{
		$options['vertical']="top";
	}
	if(!isset($options['vertical_offset']))
	{
		$options['vertical_offset']=0;
	}
	if(!isset($options['destination_image_type']))
	{
		$options['destination_image_type']="jpg";
	}

    if (!is_file($source) || !is_readable($source)) {
        trigger_error('cannot resize image; no file exists at the given path '.
            '('.var_export($source, true).')', WARNING);
        return false;
    }
    $perms = substr(sprintf('%o', fileperms($source)), -4);
    if (imagemagick_available()) {
        $result = _imagemagick_blit_image($source, $dest, $watermark,$options);
    } else if (function_exists('imagecreatetruecolor')) {
        $result = _gd_blit_image($source, $dest, $watermark,$options);
    } else {
	trigger_error('neither ImageMagick nor GD are available; cannot '.
            'blit image', WARNING);
        return false;
    }

    // Prevent the transformation from changing the file permissions.
    clearstatcache();
    $newperms = substr(sprintf('%o', fileperms($dest)), -4);
    if ($perms != $newperms) @chmod($dest, octdec($perms));
    return $result;
}


/**
* Uses Imagemagick to blit $watermark on top of $source in $dest
* See blit_image for parameter and return details
*/
function _imagemagick_blit_image($source,$dest,$watermark,$options)
{
	
	$details=_gd_get_image($source,false);
	$w=$details['width'];
	$h=$details['height'];
	$background="";

	$x=0;
	$y=0;
	$details=_gd_get_image($watermark,false);
	$insert_w=$details['width'];
	$insert_h=$details['height'];
	$insert=$details['image'];

	$xy=_get_x_and_y($source,$watermark,$options);
	$x=$xy['x'];
	$y=$xy['y'];
	
	$convert="convert -size ".$w."x".$h." xc:skyblue ";
	$background="  ".$source." -geometry  +0+0 -composite ";
	$insert="  ".$watermark." -geometry  +".$x."+".$y." -composite ";

	$exec=$convert.$background.$insert.$dest;
	$output = array();
	$exit_status = -1;
	exec($exec, $output, $exit_status);
	if ($exit_status != 0) {
		trigger_error('Imagemagick Version 6 is required. Blit image failed: '.implode('; ', $output), WARNING);
		return false;
	}
	else
	{
		return true;
	}
	
}

/**
* Uses GD to blit $watermark on top of $source in $dest
* See blit_image for parameter and return details
*/
function _gd_blit_image($source,$dest,$watermark,$options)
{
	
	$details=_gd_get_image($source);
	$w=$details['width'];
	$h=$details['height'];
	$background=$details['image'];
	
    $img   = imagecreatetruecolor( $w, $h );

	$details=_gd_get_image($watermark);
	$insert_w=$details['width'];
	$insert_h=$details['height'];
	$insert=$details['image'];

	$xy=_get_x_and_y($source,$watermark,$options);
	$x=$xy['x'];
	$y=$xy['y'];
	
	imagecopy($img,$background,0,0,0,0,$w,$h);
	imagecopy($img,$insert,$x,$y,0,0,$insert_w,$insert_h);
	
	$details=_gd_get_image($dest,false);
	switch($details['type'])
	{
  		case 'gif':
  			$ret=imagegif($img,$dest,100);
  		break;
  		case 'jpg':
  			$ret=imagejpeg($img,$dest,100);
  		break;
  		case 'png':
  			$ret=imagepng($img,$dest,0);
  		break;
		
	}

	//clean up your mess
	imagedestroy($img);
	imagedestroy($background);
	imagedestroy($insert);
	
	return $ret;
}

/**
* Used by the blit_image functions to determine
* where to start blitting $watermark onto $source
* See blit_image for an explanation of the params
* @return an array of the coordinates to start blitting
*         Keys are 'x' and 'y' in the return array
*/
function _get_x_and_y($source,$watermark,$options)
{
	$details=_gd_get_image($source,false);
	$w=$details['width'];
	$h=$details['height'];

	$details=_gd_get_image($watermark,false);

	$insert_w=$details['width'];
	$insert_h=$details['height'];
	$x=0;
	$y=0;
	
	if($options['horizontal']=="left")
	{
		$x=0+$options['horizontal_offset'];
	}
	elseif($options['horizontal']=="center")
	{
		$x=($w- $insert_w)*0.5+$options['horizontal_offset'];
	}
	elseif($options['horizontal']=="right")
	{
		$x=($w- $insert_w)+$options['horizontal_offset'];
	}
	else
	{
		$x=0;
	}
	
	if($options['vertical']=="left")
	{
		$y=0+$options['vertical_offset'];
	}
	elseif($options['vertical']=="center")
	{
		$y=($h- $insert_h)*0.5+$options['vertical_offset'];
	}
	elseif($options['vertical']=="bottom")
	{
		$y=($h- $insert_h)+$options['vertical_offset'];
	}
	else
	{
		$y=0;
	}
	$ret=array('x'=>$x,'y'=>$y);
	return $ret;
	
}

/**
* used by the blit_image functions to retrieve information
* about an image
* @param $source is the path and file we want to know about
* @param $return_image is a boolean, if true it returns a GD image
*        object with the return array
* @return an array with the following keys:
*         'width' the width of the image in pixels
*         'height' the height of the image in pixels
*         'image' a GD image object if $return_image is true, null otherwise
*         'type' the type image $source is
*/
function _gd_get_image($source,$return_image=true)
{
  	$size = getimagesize($source);
  	$w = $size[0];
  	$h = $size[1];

	$stype="";
 	$type=strtolower(substr($source,-3));
	if($type=="gif" || $type=="jpg" || $type=="png")
	{
		$stype=$type;
	}
	else
	{
		trigger_error("$type is not a supported image type file name must have suffix 'gif', 'jpg' or 'png");
	}
	
	$img=null;
	if($return_image)
	{
  		switch($stype) 
		{
  			case 'gif':
  				$img = imagecreatefromgif($source);
  			break;
  			case 'jpg':
  				$img = imagecreatefromjpeg($source);
  			break;
  			case 'png':
  				$img = imagecreatefrompng($source);
  			break;
  		}
	}
	$details=array();
	$details['width']=$w;
	$details['height']=$h;
	$details['image']=$img;
	$details['type']=$stype;
	
	return $details;
}

/**
 * Checks to see if ImageMagick is available on the system.
 *
 * This function requires the {@link IMAGEMAGICK_PATH} constant to have been
 * defined; it only checks for the command-line ImageMagick utilities, not any
 * of the library's PHP bindings.
 *
 * @param string $utility the specific ImageMagick utility to check for
 * @return boolean true if the utility is available, false if otherwise
 * @link http://www.imagemagick.org/ ImageMagick
 */
function imagemagick_available($utility='mogrify')
{
    static $cache = array();
    
    if (!isset($cache[$utility])) {
        $file = (server_is_windows()) ? "$utility.exe" : $utility;
        
        if (!@constant('IMAGEMAGICK_PATH')) {
            $result = false;
        } else if (!is_executable(IMAGEMAGICK_PATH.$file)) {
            $result = false;
        } else {
            $result = true;
        }
        
        $cache[$utility] = $result;
    }
    
    return $cache[$utility];
}

/**
 * @access private
 */
function _imagemagick_resize($path, $width, $height, $sharpen)
{
    if (defined('IMAGEMAGICK_PATH')) {
		$exec = (substr(IMAGEMAGICK_PATH, -1) == DIRECTORY_SEPARATOR)
			? IMAGEMAGICK_PATH.'mogrify'
			: IMAGEMAGICK_PATH.DIRECTORY_SEPARATOR."mogrify";
	} else {
		$exec = "mogrify";
	}
	
	$args = array(
		$exec,
		'-geometry',
		"{$width}x{$height}",
	);
	if ($sharpen)
	    $args = array_merge($args, array('-sharpen', '1'));
	$args[] = escapeshellarg($path);
	
	$output = array();
	$exit_status = -1;
	exec(implode(' ', $args), $output, $exit_status);
	if ($exit_status != 0) {
		trigger_error('image resize failed: '.implode('; ', $output), WARNING);
		return false;
	}
	
	return true;
}

/**
 * @access private
 */
function _gd_resize($path, $width, $height, $sharpen)
{
    static $image_types = array(
        1 => 'gif',
        2 => 'jpeg',
        3 => 'png'
    );
    
    $info = getimagesize($path);
    if (!$info || !isset($image_types[$info[2]])) {
        trigger_error('image resize failed: image '.var_export($path, true).
            ' is not an image or is an image in an unsupported format');
    }
    list($src_width, $src_height, $src_type) = $info;
    $type_ext = $image_types[$src_type];
    
    if ($src_width > $width || $src_height > $height) {
        $ratio = ((float) $src_width) / $src_height;
        if (((float) $width) / $height > $ratio) {
            $width = $height * $ratio;
        } else {
            $height = $width / $ratio;
        }
    }
    
    $create = "imagecreatefrom$type_ext";
    $save = "image$type_ext";
    
    // Resample
	$image_dest = imagecreatetruecolor($width, $height);
	$image_src = $create($path);
	imagecopyresampled($image_dest, $image_src, 0, 0, 0, 0, $width, $height,
	    $src_width, $src_height);
	imagedestroy($image_src);
	
	// Sharpen
	if ($sharpen) {
	    $image_dest = sharpen_image($image_dest, 80, 0.5, 3);
	}
	// Output
	$success = $save($image_dest, $path);
	imagedestroy($image_dest);
	return $success;
}

/**
 * Gets the components of an RGB color.
 * @param int $color an RGB color
 * @return array [red, green, blue]
 */
function get_color_components($color)
{
    $red = (($color >> 16) & 0xFF);
    $green = (($color >> 8) & 0xFF);
    $blue = ($color & 0xFF);
    return array($red, $green, $blue);
}


/**
 * Sharpens a GD image using an unsharp mask.
 * The given image will be sharpened in-place; this function returns the same
 * image resource as it was passed.
 * @param resource $img a GD image resource
 * @param int $amount (typically 50 - 200)
 * @param int $radius (typically 0.5 - 1)
 * @param int $threshold (typically 0 - 5)
 * @return resource the sharpened image as a GD image resource
 * @author Torsteon Hønsi <thoensi@netcom.no>
 * @version 2.1
 * @link http://en.wikipedia.org/wiki/Unsharp_masking the technique used
 * @link http://vikjavev.no/computing/ump.php the source of this code
 */
function sharpen_image($img, $amount, $radius, $threshold)
{
    // Attempt to calibrate the parameters to Photoshop:
    $amount = min($amount, 500) * 0.016;
    $radius = min($radius, 50) * 2;
    $threshold = min($threshold, 255);

    $radius = abs(round($radius));
    if ($radius == 0) {
        trigger_error('unsharp radius is 0; not performing any sharpening',
            WARNING);
        return $img;
    }

    list($width, $height) = array(imagesx($img), imagesy($img));
    $canvas = imagecreatetruecolor($width, $height);
    $blur = imagecreatetruecolor($width, $height);

    static $gaussian_blur_matrix = array(
        array(1, 2, 1),
        array(2, 4, 2),
        array(1, 2, 1)
    );
    if (function_exists('imageconvolution')) {
        imagecopy($blur, $img, 0, 0, 0, 0, $width, $height);
        imageconvolution($blur, $gaussian_blur_matrix, 16, 0);
    } else {
        // Move copes of the image around one pixel at a time and merge them
        // with weight according to the matrix. The same matrix is simply
        // repeated for higher radii.
        for ($i = 0; $i < $radius; $i++) {
            imagecopy($blur, $img, 0, 0, 1, 0, $w - 1, $h); // left
            imagecopymerge($blur, $img, 1, 0, 0, 0, $w, $h, 50); // right
            imagecopymerge($blur, $img, 0, 0, 0, 0, $w, $h, 50); // center
            imagecopy($canvas, $blur, 0, 0, 0, 0, $w, $h);

            imagecopymerge($blur, $canvas, 0, 0, 0, 1, $w, $h - 1,
                33.33333); // up
            imagecopymerge($blur, $canvas, 0, 1, 0, 0, $w, $h, 25); // down
        }
    }

    if ($threshold > 0) {
        for ($x = 0; $x < $width - 1; $x++) {
            for ($y = 0; $y < $height; $y++) {
                $rgb = imagecolorat($img, $x, $y);
                list($rOrig, $gOrig, $bOrig) = get_color_components($rgb);

                $rgb = imagecolorat($blur, $x, $y);
                list($rBlur, $gBlur, $bBlur) = get_color_components($rgb);

                // When the masked pixels differ less from the original than
                // the threshold specifies, they are set to their original
                // value.
                $rNew = (abs($rOrig - $rBlur) >= $threshold)
                    ? _bound(($amount * ($rOrig - $rBlur)) + $rOrig, 0, 255)
                    : $rOrig;
                $gNew = (abs($gOrig - $gBlur) >= $threshold)
                    ? _bound(($amount * ($gOrig - $gBlur)) + $gOrig, 0, 255)
                    : $gOrig;
                $bNew = (abs($bOrig - $bBlur) >= $threshold)
                    ? _bound(($amount * ($bOrig - $bBlur)) + $bOrig, 0, 255)
                    : $bOrig;

                if ($rOrig != $rNew || $gOrig != $gNew || $bOrig != $bNew) {
                    $pixCol = imagecolorallocate($img, $rNew, $gNew, $bNew);
                    imagesetpixel($img, $x, $y, $pixCol);
                }
            }
        }
    } else {
        for ($x = 0; $x < $width; $x++) {
            for ($y = 0; $y < $height; $y++) {
                $rgb = imagecolorat($img, $x, $y);
                list($rOrig, $gOrig, $bOrig) = get_color_components($rgb);

                $rgb = imagecolorat($blur, $x, $y);
                list($rBlur, $gBlur, $bBlur) = get_color_components($rgb);

                $rNew = _bound(($amount * ($rOrig - $rBlur)) + $rOrig, 0, 255);
                $gNew = _bound(($amount * ($gOrig - $gBlur)) + $gOrig, 0, 255);
                $bNew = _bound(($amount * ($bOrig - $bBlur)) + $bOrig, 0, 255);

                $rgbNew = ($rNew << 16) + ($gNew << 8) + $bNew;
                imagesetpixel($img, $x, $y, $rgbNew);
            }
        }
    }

    imagedestroy($canvas);
    imagedestroy($blur);

    return $img;
}

/**
 * Restricts $val such that $min ≤ $val ≤ $max.
 * @access private
 */
function _bound($val, $min, $max)
{
    return max($min, min($val, $max));
}
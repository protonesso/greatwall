#!/usr/bin/perl
#
# This code is distributed under the terms of the GPL
#
# (c) 2004-2008 marco.s - http://www.urlfilter.net
#
# $Id: redirect.cgi,v 0.3 2008/01/23 00:00:00 marco.s Exp $
#

$swroot="/var/smoothwall";
use lib "/usr/lib/smoothwall";
use header qw( :standard );


$cgiquery=$ENV{QUERY_STRING}.'&';

my %netsettings;
my %filtersettings;
my %params;

@a=split(/&/,$cgiquery);
for ($i=0; $i<=@a; $i++) {
        @b=split(/=/,$a[$i]);
        $params{$b[0]}=$b[1];
}

$category=$params{'category'};
$url=$params{'url'};
$ip=$params{'ip'};

&readhash("$swroot/ethernet/settings", \%netsettings);
if (-e "$swroot/urlfilter/settings") { &readhash("$swroot/urlfilter/settings", \%filtersettings); }

if ($filtersettings{'MSG_TEXT_1'} eq '') {
	$msgtext1 = "ACCESS DENIED";
} else { $msgtext1 = $filtersettings{'MSG_TEXT_1'}; }
if ($filtersettings{'MSG_TEXT_2'} eq '') {
	$msgtext2 = "Access to the requested page has been denied.";
} else { $msgtext2 = $filtersettings{'MSG_TEXT_2'}; }
if ($filtersettings{'MSG_TEXT_3'} eq '') {
	$msgtext3 = "Please contact the Network Administrator if you think there has been an error.";
} else { $msgtext3 = $filtersettings{'MSG_TEXT_3'}; }

if ($category eq '') { $category = '&nbsp;'; } else { $category = '['.$category.']'; }

&showhttpheaders();

print <<END
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
  <title>Firewall Denied Access</title>
  <script language='javascript' SRC='http://$netsettings{'GREEN_ADDRESS'}:81/ui/js/script.js'></script>
  <link href='http://$netsettings{'GREEN_ADDRESS'}:81/ui/css/style.css' rel='stylesheet' type='text/css'>
</head>

END
;



if (($filtersettings{'ENABLE_JPEG'} eq 'on') && (-e "/httpd/html/urlfilter/background.jpg"))
{
print <<END
<body style="background-image:URL('http://$netsettings{'GREEN_ADDRESS'}:81/urlfilter/background.jpg'); background-color:#FFFFFF; font-family:verdana, arial, 'sans serif';">
END
;
} else {
print <<END
<body style="background-color:#FFFFFF; font-family:verdana, arial, 'sans serif';">
END
;
}

print <<END
<table class='frame' cellpadding='0' cellspacing='0'>
<tr style="margin:0">
  <td style="background-image:url(http://$netsettings{'GREEN_ADDRESS'}:81/ui/img/top-left.png); max-height:4px; height:4px; min-height:4px; max-width:4px; min-width:4px; background-repeat:none"></td>
    <td style="background-image:url(http://$netsettings{'GREEN_ADDRESS'}:81/ui/img/top.png); max-height:4px; height:4px; min-height:4px; background-repeat:repeat-x"></td>
      <td style="background-image:url(http://$netsettings{'GREEN_ADDRESS'}:81/ui/img/top-right.png); max-height:4px; height:4px; min-height:4px; max-width:4px; width:4px; min-width:4px; background-repeat:none"></td>
      </tr>
      <tr style="margin:0">
        <td style="background-image:url(http://$netsettings{'GREEN_ADDRESS'}:81/ui/img/left.png); max-width:4px; width:4px; min-width:4px; background-repeat:repeat-y">
	  </td>
	    <td>

<div style='margin:0'>
  <div class='header'
       style='background-image:url(http://$netsettings{'GREEN_ADDRESS'}:81/ui/img/banner.png);
              width:800px; height:64px; margin 45px 10em 0 20em'>
    <p style='margin:0; padding:42px 0 4px 575px;color:#eeeeee;font-weight:bold'>
      Web Filtering by <a style="color:#eeeeee;"
                          onmouseover="this.style.color='#208020'"
                          onmouseout="this.style.color='#eeeeee'"
                          href="http://www.squidguard.org"><b>SquidGuard</b></a>
    </p>
  </div>
  <div style="padding:5px; background-color:#C0C0C0; text-align:center;
              font-weight:bold; font-family:verdana,arial,'sans serif';
	      margin:0; color:#000000; font-size:60%;">
		$category
  </div>
	<div style="background-color:#F4F4F4; text-align:center; padding:20px;">
    <div style="letter-spacing:0.5em; word-spacing:1em; padding:20px; background-color:#FF0000; text-align:center; color:#FFFFFF; font-size:200%; font-weight: bold;">
      $msgtext1
    </div>
    <div style="padding:20px; margin-top:20px; background-color:#E2E2E2; text-align:center; color:#000000; font-family:verdana, arial, 'sans serif'; font-size:80%;">
      <p style="font-weight:bold; font-size:150%;">$msgtext2</p>
END
;

if (!($url eq ""))
{
print <<END
      <p>URL: <a href="$url">$url</a></p>
END
;
}

if (!($ip eq ""))
{
print <<END
      <p>Client IP address: <span style="font-style:italic;">$ip</span></p>
END
;
}

print <<END
      <p>$msgtext3</p>
    </div>
	</div>
</div>

    </td>
    <td style="background-image:url(http://$netsettings{'GREEN_ADDRESS'}:81/ui/img/right.png); max-width:6px; width:6px; min-width:6px; background-repeat:repeat-y">
    </td>
  </tr>
  <tr>
    <td style="background-image:url(http://$netsettings{'GREEN_ADDRESS'}:81/ui/img/bottom-left.png); max-height:6px; height:6px; min-height:6px; max-width:4px; min-width:4px; background-repeat:none">
    </td>
    <td style="background-image:url(http://$netsettings{'GREEN_ADDRESS'}:81/ui/img/bottom.png); max-height:6px; height:6px; min-height:6px; background-repeat:repeat-x">
    </td>
    <td style="background-image:url(http://$netsettings{'GREEN_ADDRESS'}:81/ui/img/bottom-right.png); max-height:6px; height:6px; min-height:6px; max-width:6px; width:6px; min-width:6px; background-repeat:none">
    </td>
  </tr>
</table>
</body>
</html>
END
;

sub readhash
{
	my $filename = $_[0];
	my $hash = $_[1];
	my ($var, $val);

	if (-e $filename)
	{
		open(FILE, $filename) or die "Unable to read file $filename";
		while (<FILE>)
		{
			chop;
			($var, $val) = split /=/, $_, 2;
			if ($var)
			{
				$val =~ s/^\'//g;
				$val =~ s/\'$//g;
	
				# Untaint variables read from hash
				$var =~ /([A-Za-z0-9_-]*)/;        $var = $1;
				$val =~ /([\w\W]*)/; $val = $1;
				$hash->{$var} = $val;
			}
		}
		close FILE;
	}
}

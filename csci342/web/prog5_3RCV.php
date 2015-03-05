<?php
	$apiResponse = file_get_contents('https://itunes.apple.com/search?term=jack+johnson');
	// print $apiResponse;
	$obj = utf8_decode($apiResponse);
	$results = json_decode($obj);
	// $icount = count($results);
	// print $icount;
	$i = 1;
	while ($i < 60) 
	{
		$preview = $results->results[$i]->trackName;
		print $preview;
		$i = $i + 1;
		print '<br>';
	}
		
	
	
	
	
	
	
	
	
	
	// $allResults = $json->results;
	// $appInfo = $allResults[0];
	
	//Load any value into a variable by specifing a key e.g. "sellerName"
	// $sellerName = $appInfo->artistID;
	
	//Use Variables
	// echo $sellerName;
?>
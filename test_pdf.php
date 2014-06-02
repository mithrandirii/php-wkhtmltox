<?php
	$errorlog = array();
	$success = wkhtmltox_convert(
		'pdf', // output format pdf/image
		array( // global settings
			'out' => './out.pdf', // file output
			//'out' => NULL, // string output
			//'margin.top' => 0,
			//'margin.right' => 0,
			//'margin.bottom' => 0,
			//'margin.left' => 0
		),
		array( // object settings
			array( // first object
				'page' => './test.html', // file input
				//'page' => NULL, // 'html' for string input
				//'html' => '<html><body><h1>test : ' . strftime('%Y-%m-%d %H:%M:%S') . '</h1></body></html>' // string input
			)/*,
			array( // second object
				'page' => 'http://www.google.de' // url input
			)*/
		),
		$errorlog
	);
	
	if ($success) {
		$output = is_string($success) ? $success : NULL;
		if (!empty($output)) {
			echo $output;
		} else {
			echo 'successful.' . "\n";
		}
		exit (0);
	} else {
		echo 'error occurred ...' . "\n";
		foreach ($errorlog as $log) {
			 echo $log['status'] . ': ' . $log['message'] . "\n";
		}
		exit (1);
	}
?>

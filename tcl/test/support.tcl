TestSuite instproc June01defaults {} {
	 Agent/TCP set windowInit_ 2
	 Agent/TCP set singledup_ 1
	 Agent/TCP set minrto_ 1
	 Agent/TCP set syn_ true
	 Agent/TCP set delay_growth_ true
}

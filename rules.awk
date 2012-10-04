# AWK script to add Makefile rules to "cc -M" output.

BEGIN {
	if (rule == "") {
		print "ERROR: you must set \"rule\" from command-line."
		exit 1
	}
	inrule = 0
}

$1 ~ /\.o:$/	{
	print

	if ($NF == "\\") {
		inrule = 1
		next
	} else
		print rule
}

$1 !~ /\.o:$/	{
	print
	if (inrule) {
		if ($NF == "\\")
			next
		else {
			print rule
			inrule = 0
		}
	}
}

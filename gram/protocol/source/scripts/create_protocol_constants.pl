print <<EOF;
/* This file is automatically generated by
 * $0. Do not modify.
 */

#ifndef GLOBUS_GRAM_PROTOCOL_CONSTANTS_H
#define GLOBUS_GRAM_PROTOCOL_CONSTANTS_H

/**
 * \@defgroup globus_gram_protocol_constants GRAM Protocol Constants
 */
EOF

my $type = 0;
my $comma_nl="";
while(<>)
{
    next if(/^#[^#]/);
    if(/^##[^#]/ && $type)
    {
	print "\n}\n$type;\n";
	$comma_nl = "";
	$documentation = "";
    }
    if(/^##[^#]/)
    {
	chomp;
	($type, $typedoc) = (split(/\s+/, $_, 3))[1,2] ;
	print <<EOF;
/** $typedoc
 * \@ingroup globus_gram_protocol_constants
 */
typedef enum
{
EOF
	$documentation = "";
	next;
    }
    if(/^###\s*(.*)/)
    {
	$documentation .= " $1";
	next;
    }
    if(/=/)
    {
	chomp;
	my @pair = split(/=/);
	print "$comma_nl    $pair[0] = $pair[1]";
	if($documentation ne "")
	{
	    print " /**< $documentation */";
	    $documentation = "";
	}
	$comma_nl = ",\n";
    }
}

print "\n}\n$type;\n#endif\n";

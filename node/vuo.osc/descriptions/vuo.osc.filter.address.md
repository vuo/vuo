Only lets an OSC message pass through if its address matches the `Address` input port.

This node uses [case-sensitive wildcard matching](vuo-nodeset://vuo.text).  For example, if the `Address` filter is `/sol/*`, an incoming OSC message with address `/sol/earth` would match, but a message with address `/tau-ceti/f` would not match.

Outputs *true* if both input ports receive the same event, and *false* otherwise. 

Events fired by different trigger ports (even if they're nearly simultaneous) will not cause the node to output *true*, since they're not the same event. 

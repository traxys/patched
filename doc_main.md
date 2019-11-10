**Lines** is a way to extract lines from a file

**Searcher** is what finds the path in the patch graph, using Path and Cost

**create_patch** is what creates a patch from a path in the patch graph

There is an invariant mainted by the Searcher: it inserts a cost in only if it is the shortest Path to go there.

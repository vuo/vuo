Turns JSON-formatted text into a tree.

If there's an error in the JSON, this node outputs an empty tree. Check Vuo's Console window (Tools > Show Console) for details about the error.

Name-value pairs in JSON objects roughly correspond to trees.

A name-value pair whose value is a string, number, boolean, or null is turned into a tree whose name is the name from the pair and whose content is the value, like this:

    { "title" : "China Mountain Zhang" }

   - Tree named "title" with content "China Mountain Zhang"

A name-value pair whose value is an object is turned into a tree with children, like this:

    { "book" :
       { "title" : "China Mountain Zhang",
         "author" : "Maureen F. McHugh"
       }
    }

   - Tree named "book" with children:
      - Tree named "title" with content "China Mountain Zhang"
      - Tree named "author" with content "Maureen F. McHugh"

A name-value pair whose value is an array is turned into a tree with children whose names are the name from the pair, like this:

    { "book" :
       { "award" :
          [ "James Tiptree, Jr. Memorial Award",
            "Lambda Literary Award",
            "Locus Award for Best First Novel"
          ]
       }
    }

   - Tree named "book" with children:
      - Tree named "award" with content "James Tiptree, Jr. Memorial Award"
      - Tree named "award" with content "Lambda Literary Award"
      - Tree named "award" with content "Locus Award for Best First Novel"

If the outermost JSON is something other than an object containing a single name-value pair, a parent tree is added when transforming the JSON to a tree, like this:

    "Mandarin"

   - Tree with no name and content "Mandarin"

<!-- -->

    [ 1992, 1994, 1998, 2001 ]

   - Tree with no name and children:
      - Tree with no name and content "1992"
      - Tree with no name and content "1994"
      - Tree with no name and content "1998"
      - Tree with no name and content "2001"

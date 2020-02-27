Turns a tree (including descendants) into JSON-formatted text.

   - `Indent` — If true, the output will include spacing and linebreaks so it's easier to read. If false, the output will have no whitespace between items.

If the tree was created with text fed into the [Make Tree from JSON](vuo-node://vuo.tree.make.json) node, this node outputs the original JSON information, preserving the data types of JSON numbers, booleans, and null values. Otherwise, the tree is translated to JSON format, with all content translated to JSON strings.

A tree that has just a name and content, no attributes or children, is turned into a JSON object, like this:

   - Tree named "title" with content "China Mountain Zhang"

<!-- -->

    { "title" : "China Mountain Zhang" }

If a tree has children, and each child has a unique name, the tree is turned into a JSON object with sub-objects, like this:

   - Tree named "book" with children:
      - Tree named "title" with content "China Mountain Zhang"
      - Tree named "author" with content "Maureen F. McHugh"

<!-- -->

    { "book" :
       { "title" : "China Mountain Zhang",
         "author" : "Maureen F. McHugh"
       }
    }

If some of the tree's children have the same name, they're turned into a JSON array, like this:

   - Tree named "book" with children:
      - Tree named "award" with content "James Tiptree, Jr. Memorial Award"
      - Tree named "award" with content "Lambda Literary Award"
      - Tree named "award" with content "Locus Award for Best First Novel"

<!-- -->

    { "book" :
       { "award" :
          [ "James Tiptree, Jr. Memorial Award",
            "Lambda Literary Award",
            "Locus Award for Best First Novel"
          ]
       }
    }

A tree's attributes, like its children, are turned into JSON sub-objects, like this:

   - Tree named "book" with attribute "isbn = 0312852711" and children:
      - Tree named "publisher" with content "Tor Books"

<!-- -->

    { "book" :
       { "isbn" : "0312852711",
         "publisher" : "Tor Books"
       }
    }

If a tree or any of its descendants lack names, names are added, like this:

   - Tree with no name and content "Mandarin"

<!-- -->

    { "document" : "Mandarin" }

   - Tree with no name and children:
      - Tree with no name and content "1992"
      - Tree with no name and content "1994"
      - Tree with no name and content "1998"
      - Tree with no name and content "2001"

<!-- -->

    { "document":
       "item": [ 1992, 1994, 1998, 2001 ]
    }

Turns a tree (including descendants) into XML-formatted text.

   - `Indent` — If true, the output will include spacing and linebreaks so it's easier to read. If false, the output will have no whitespace between elements.

The tree's name, attributes, and content become the XML root element's name, attributes, and content. The tree's children become the XML root element's child elements.

If a tree or any of its descendants lack names, names are added, like this:

   - Tree with no name and content "Mandarin"

<!-- -->

    <document>Mandarin</document>

   - Tree with no name and children:
      - Tree with no name and content "1992"
      - Tree with no name and content "1994"
      - Tree with no name and content "1998"
      - Tree with no name and content "2001"

<!-- -->

    <document>
       <item>1992</item>
       <item>1994</item>
       <item>1998</item>
       <item>2001</item>
    </document>

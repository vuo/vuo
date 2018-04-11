Finds all items in a tree that match an [XPath expression](http://www.w3schools.com/xml/xml_xpath.asp).

For example, if the tree was created from the following XML…

    <bookstore>
       <book isbn="0140448063" sku="P456">
          <title>The Pillow Book</title>
          <author>Sei Shonagon</author>
          <translator>Meredith McKinney</translator>
          <price>12</price>
       </book>
       <book isbn="0765627345" sku="N123">
          <title>More Stories by Japanese Women Writers</title>
          <editor>Kyoko Selden</editor>
          <editor>Noriko Mizuta</editor>
          <price>19</price>
       </book>
    </bookstore>

…then this node's output for each of the following XPath expressions would be…

   - `/bookstore/book/title` — Two trees, one representing `<title>The Pillow Book</title>` and one representing `<title>More Stories by Japanese Women Writers</title>`.
   - `book/title` — Same as previous.
   - `//title` — Same as previous.
   - `/bookstore/book/editor[1]` — A tree representing `<editor>Kyoko Selden</editor>`.
   - `/bookstore/book/price[. < 15]` — A tree representing `<price>12</price>`.
   - `/bookstore/book[translator]` — A tree representing the first `<book>` element, including its attributes, content, and children.
   - `/bookstore/book[@isbn="0140448063"]` — Same as previous.
   - `/bookstore/book/@sku` — Two trees, one with name `book` and attribute `sku="P456"` and one with name `book` and attribute `sku="N123"`. Neither tree has content or children.
   - `/bookstore/book/@*` — Two trees, one with name `book` and attributes `isbn="0140448063" sku="P456"` and one with name `book` and attributes `isbn="0765627345" sku="N123"`. Neither tree has content or children.

If there's an error in the XPath expression, this node outputs an empty list. Check Console.app for details about the error.

These nodes are for working with information that has a hierarchical (tree) structure.

One example of a tree structure is the biological classification of animals. Animals are grouped by their common features and ancestry, like this:

   - *Hominidae*
      - *Gorilla*
         - *G. beringei* — eastern gorilla
         - *G. gorilla* — western gorilla
      - *Homo*
         - *H. sapiens* — human
      - *Pan*
         - *P. paniscus* — bonobo
         - *P. troglodytes* — chimpanzee

In computing, information with a tree structure is often expressed in [XML](http://www.w3schools.com/xml/xml_whatis.asp) or [JSON](http://www.json.org/). Both of these formats are supported by the `vuo.tree` nodes.

The biological classification above could be represented in XML in various ways, including this:

    <family>
       <name>Hominidae</name>
       <genus>
          <name>Gorilla</name>
          <species>
             <name>G. beringei</name>
             <common-name>eastern gorilla</common-name>
          </species>
          <species>
             <name>G. gorilla</name>
             <common-name>western gorilla</common-name>
          </species>
       </genus>
       <genus>
          <name>Homo</name>
          <species>
             <name>H. sapiens</name>
             <common-name>human</common-name>
          </species>
       </genus>
       <genus>
          <name>Pan</name>
          <species>
             <name>P. paniscus</name>
             <common-name>bonobo</common-name>
          </species>
          <species>
             <name>P. troglodytes</name>
             <common-name>chimpanzee</common-name>
          </species>
       </genus>
    </family>

Equivalently, the biological classification could be represented in JSON like this:

    { "family" :
       { "name" : "Hominidae",
         "genus" :
            [ { "name" : "Gorilla",
                "species" :
                   [ { "name": "G. beringei",
                       "common-name": "eastern gorilla"
                     },
                     { "name": "G. gorilla",
                       "common-name": "western gorilla"
                     } ]
              },
              { "name" : "Homo",
                "species" :
                   [ { "name": "H. sapiens",
                       "common-name": "human"
                     } ]
              },
              { "name" : "Pan",
                "species" :
                   [ { "name": "P. paniscus",
                       "common-name": "bonobo"
                     },
                     { "name": "P. troglodytes",
                       "common-name": "chimpanzee"
                     } ]
              } ]
       }
    }

If you were to feed the XML-formatted text above into the `Make Tree from XML` node or the JSON-formatted text above into the `Make Tree from JSON` node, the node would output data of type Tree that is structured like this:

   - Tree named "family" with children:
      - Tree named "name" with content "Hominidae"
      - Tree named "genus" with children:
         - Tree named "name" with content "Gorilla"
         - Tree named "species" with children:
            - Tree named "name" with content "G. beringei"
            - Tree named "common-name" with content "eastern gorilla"
         - Tree named "species" with children:
            - Tree named "name" with content "G. gorilla"
            - Tree named "common-name" with content "western gorilla"
      - Tree named "genus" with children:
         - Tree named "name" with content "Homo"
         - Tree named "species" with children:
            - Tree named "name" with content "H. sapiens"
            - Tree named "common-name with content "human"
      - Tree named "genus" with children:
         - Tree named "name" with content "Pan"
         - Tree named "species" with children:
            - Tree named "name" with content "P. paniscus"
            - Tree named "common-name with content "bonobo"
         - Tree named "species" with children:
            - Tree named "name" with content "P. troglodytes"
            - Tree named "common-name with content "chimpanzee"

The `Make Tree from XML` and `Make Tree from JSON` nodes extract the essential tree structure from the text, making it possible for you to use the same set of Tree nodes regardless of whether the input text was formatted as JSON or XML. You can convert a Tree back to text of either format, using the `Format Tree as XML` and `Format Tree as JSON` nodes.

Data of type Tree can have:

   - a **name**, which corresponds to the element name in XML or the name in an object's name-value pair in JSON,
   - **attributes**, which correspond to the attributes of an element in XML (explained below),
   - **content**, which corresponds to the text enclosed by an element in XML or the value in an object's name-value pair in JSON, and
   - **children**, each of which is also a Tree.

A Tree can have many levels — children, grandchildren, and so on. The Tree's children, grandchildren, and so on are called the Tree's **descendants**.

A Tree's name, attributes, and content refer to just one level of the Tree. Each child Tree has its own name, attributes, and content. For example, the first Tree in the list above has the name "family" and no content of its own. One of its great-grandchildren has the name "common-name" and does have content, "eastern gorilla".

In XML, an element can have attributes attached to it, like this:

    <species status="endangered" ncbi-genome-id="10729">

Each attribute is a name-value pair, like "status" and "endangered". Like an XML element, a Vuo Tree can have multiple attributes.

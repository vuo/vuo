Tries to find and decode a barcode in an image.

This node can detect many types of barcodes, including:

   - QR Code
   - DataMatrix
   - UPC and EAN
   - Code 39, 93, and 128
   - Codabar
   - ITF
   - GS1/RSS
   - Aztec
   - PDF 417

If a barcode is detected, the event passes through, and the decoded barcode text, center, and size are output.  If no barcode is found, the event is blocked.

Since this is a computationally intensive operation, you'll probably want to set the Event Throttling mode of the trigger port driving this node to "Drop Events".

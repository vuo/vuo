Tries to find and decode a barcode in an image.

This node can detect many types of barcodes, including:

   - **1D Barcodes**
      - Codabar
      - Code 39, 93, and 128
      - EAN-8 and EAN-13
      - Interleaved 2 of 5 (ITF)
      - UPC-A and UPC-E
   - **2D Barcodes**
      - Aztec
      - Data Matrix
      - PDF417
      - QR Code

If a barcode is detected, the event passes through, and the decoded barcode text, center, and size are output.  If no barcode is found, the event is blocked.

Since this is a computationally intensive operation, you'll probably want to set the Event Throttling mode of the trigger port driving this node to "Drop Events".  If you know the specific type of barcode you will be scanning, select it in the `Format` input port, to scan for only that type (which may improve performance).

# Negative processing workflow

*vkd* can be used to process raw scans of camera negatives into graded positive images with a handful of nodes. 

- launch `vkd-photo.exe`
- navigate to your raw scan in the photo browser
- double click the thumbnail to open the node graph for that image
- the new node graph will have a read node
- click "invert" to add an invert node
- click "cdl" to add a colour decision list node

Missing options:
- Currently no well-defined working space, so all OCIO operations have to be specified both to and from a space.
- Input node can't apply an OCIO transform.
- CDL node can't apply an OCIO transform before and after operations.
- No LUT support at present.

<p align="right">(<a href="#top">back to top</a>)</p>
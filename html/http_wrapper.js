/*
 * httpXmlWrapper: Performs a HTTP GET request for the given URL by using a XMLHttpRequest.
 * Displays an error message when something goes wrong.
 * Returns the result already parsed as XML.
 */
  
function httpXmlWrapper(url, xmlCallback)
{
    function xhreqCallback()
    {
        if (this.readyState == 4)
	{
	    if (this.status == 200)
                xmlCallback(new DOMParser().parseFromString(this.responseText, "application/xml"));
	    else
	        alert(this.statusText);
	}
    }
          
    var xhreq = new XMLHttpRequest();
    if (xhreq == null)
	alert("Browser not supported (the browser doesn't have XMLHttpRequest). "
	    + "Please use instead e.g. a recent Firefox version.");
	
    xhreq.open("GET", url, true);
    xhreq.onreadystatechange = xhreqCallback;
    xhreq.send(null);    
}

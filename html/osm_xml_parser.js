/*
 * OsmXmlParser: an object that has only the method:
 *   processXML: parses the given DOM tree into an array of OSM objects
 */

function OsmXmlParser()
{
    function collect_tags(elem)
    {
        var result = {};
        
        if (elem && elem.children)
        {
            for (var i = 0; i < elem.children.length; ++i)
            {
                var tag = elem.children.item(i);
                if (tag && tag.attributes && tag.attributes.getNamedItem("k") && tag.attributes.getNamedItem("v"))
		    result[tag.attributes.getNamedItem("k").value] = tag.attributes.getNamedItem("v").value;
            }
        }
        
        return result;
    }
    
    
    function collect_geometry(elem)
    {
        var result = {};
	
	if (elem && elem.nodeName == "node")
	{
	    if (elem && elem.attributes && elem.attributes.getNamedItem("lon"))
	      result["x"] = elem.attributes.getNamedItem("lon").value;
	    if (elem && elem.attributes && elem.attributes.getNamedItem("lat"))
	      result["y"] = elem.attributes.getNamedItem("lat").value;
	}
	
	return result;
    }
    
    
    function append_attribute_if_available(source, result, attr)
    {
	if (source.attributes && source.attributes.getNamedItem(attr))
	    result[attr] = source.attributes.getNamedItem(attr).value;
    }
    
    
    function parse(xml_node)
    {
        var result = {};
	result["type"] = xml_node.nodeName;
	append_attribute_if_available(xml_node, result, "id");
	append_attribute_if_available(xml_node, result, "version");
	append_attribute_if_available(xml_node, result, "timestamp");
	append_attribute_if_available(xml_node, result, "changeset");
	append_attribute_if_available(xml_node, result, "uid");
	append_attribute_if_available(xml_node, result, "user");
	result["tags"] = collect_tags(xml_node);
	result["geometry"] = collect_geometry(xml_node);
	return result;
    }
    
    
    function processElemClass(elem_name, doc, result)
    {
        var elem_list = doc.getElementsByTagName(elem_name);
        for (var i = 0; i < elem_list.length; ++i)
        {
            if (elem_list[i] && elem_list[i].parentNode && elem_list[i].parentNode.nodeName == "old")
            {
                var elem = {}
                elem["old"] = parse(elem_list[i]);
                result.push(elem);
            }
            else if (elem_list[i] && elem_list[i].parentNode && elem_list[i].parentNode.nodeName == "new")
            {
                if (result.length > 0 && result[result.length - 1] && result[result.length - 1].old
                        && result[result.length - 1].old.id
                        && elem_list[i].attributes
                        && elem_list[i].attributes.getNamedItem("id")
                        && elem_list[i].attributes.getNamedItem("id").value ==
                            result[result.length - 1].old.id
                        && elem_list[i].nodeName == result[result.length - 1].old.type)
                    result[result.length - 1]["new"] = parse(elem_list[i]);
                else
                {
                    var elem = {}
                    elem["new"] = parse(elem_list[i]);
                    result.push(elem);
                }
            }
            else if (elem_list[i] && elem_list[i].parentNode && elem_list[i].parentNode.nodeName == "action"
                && elem_list[i].parentNode.attributes
                && elem_list[i].parentNode.attributes.getNamedItem("type")
		&& elem_list[i].parentNode.attributes.getNamedItem("type").value == "create")
            {
                var elem = {}
                elem["new"] = parse(elem_list[i]);
                result.push(elem);
            }
            else
            {
                var elem = {}
                elem["keep"] = parse(elem_list[i]);
                result.push(elem);
            }
        }
    }

    
    function processXML(doc)
    {
        var result = [];
        processElemClass("node", doc, result);
        processElemClass("way", doc, result);
        processElemClass("relation", doc, result);    
        return result;
    }
    
    return processXML;
}

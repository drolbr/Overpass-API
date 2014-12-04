/*
 * OsmXmlParser: a closure that offers essetially an XML to OSM object parser:
 *   processXML: parses the given DOM tree into an array of OSM objects
 *   equal_geometry: returns whether both objects represent the same geometry, element order matters
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
                if (tag && tag.nodeName && tag.nodeName == "tag"
                        && tag.attributes && tag.attributes.getNamedItem("k") && tag.attributes.getNamedItem("v"))
                    result[tag.attributes.getNamedItem("k").value] = tag.attributes.getNamedItem("v").value;
            }
        }
        
        return result;
    }
    
    
    function collect_coordinate(elem)
    {
        var result = {};
        
        if (elem && elem.attributes && elem.attributes.getNamedItem("lon"))
            result["x"] = elem.attributes.getNamedItem("lon").value;
        if (elem && elem.attributes && elem.attributes.getNamedItem("lat"))
            result["y"] = elem.attributes.getNamedItem("lat").value;
        
        return result;
    }
    
    
    function collect_geometry(elem)
    {
        if (elem && elem.nodeName == "node")
            return collect_coordinate(elem);
        else if (elem && elem.nodeName == "way")
        {
            var result = [];
            var subline = [];
            
            for (var i = 0; i < elem.children.length; ++i)
            {
                var nd = elem.children.item(i);
                if (nd && nd.nodeName == "nd")
                {
                    var coord = collect_coordinate(nd);
                    if (coord.x && coord.y)
                        subline.push(coord);
                    else if (subline.length > 0)
		    {
                        result.push(subline);
			subline = [];
		    }
                }
            }
            
            if (subline.length > 0)
                result.push(subline);
            
            return result;
        }
        else if (elem && elem.nodeName == "relation")
        {
            var result = [];
            
            for (var i = 0; i < elem.children.length; ++i)
            {
                var member = elem.children.item(i);
                if (member && member.nodeName == "member" && member.attributes
		        && member.attributes.getNamedItem("type"))
                {
		    if (member.attributes.getNamedItem("type").value == "node")
		    {
                        var coord = collect_coordinate(member);
                        if (coord.x && coord.y)
                            result.push(coord);
		    }
		    else if (member.attributes.getNamedItem("type").value == "way"
		            && member.children)
		    {
			var subline = [];
			
			for (var i = 0; i < member.children.length; ++i)
			{
			    var nd = elem.children.item(i);
			    if (nd && nd.nodeName == "nd")
			    {
				var coord = collect_coordinate(nd);
				if (coord.x && coord.y)
				    subline.push(coord);
				else if (subline.length > 0)
				{
				    result.push(subline);
				    subline = [];
				}
			    }
			}
            
			if (subline.length > 0)
			    result.push(subline);
		    }
                }
            }
            
            return result;
        }
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

    
    function processXML_(doc)
    {
        var result = [];
        processElemClass("node", doc, result);
        processElemClass("way", doc, result);
        processElemClass("relation", doc, result);    
        return result;
    }
    
    
    function equal_geometry_(lhs, rhs)
    {
        if (lhs.x && lhs.y)
            return (rhs.x && rhs.y && lhs.x == rhs.x && lhs.y == rhs.y);
        if (rhs.x || rhs.y)
            return false;
        if (lhs.length != rhs.length)
            return false;
        for (var i = 0; i < lhs.length; ++i)
        {
	    if (!equal_geometry_(lhs[i], rhs[i]))
	        return false;
        }
        return true;
    }
    

    var result =
    {
        equal_geometry: equal_geometry_,
        processXml: processXML_
    }
    return result;
}

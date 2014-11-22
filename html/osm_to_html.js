
function OsmToHtml(dom_element)
{
    var target = dom_element;
    
    
    function print_tags(tags)
    {
        var result = "";
        
        if (tags)
        {
            for (var key in tags)
		result += "<br/>" + key + " = " + tags[key];
        }
        
        return result;
    }

    
    function print_tags_diff(old_tags, new_tags)
    {
        var result = "";
        
        if (old_tags && new_tags)
        {
            for (key in old_tags)
            {
                if (!new_tags[key] || old_tags[key] != new_tags[key])
                    result += "<br/><span style=\"color:red\">-&nbsp;"
		        + key + " = " + old_tags[key] + "</span>";
	    }
            
            for (key in new_tags)
            {
                if (!old_tags[key] || old_tags[key] != new_tags[key])
                    result += "<br/><span style=\"color:green\">+&nbsp;"
		        + key + " = " + new_tags[key] + "</span>";
	    }
            
            for (key in new_tags)
            {
                if (old_tags[key] && old_tags[key] == new_tags[key])
                    result += "<br/>&nbsp;&nbsp;"
		        + key + " = " + new_tags[key];
	    }
        }
        
        return result;
    }
    
    
    function print_elem(feature)
    {
        if (feature.keep)
        {
            var result = "";
        
            var elem = feature.keep;
            result += "<br/>" + elem.type + " " + elem.id;
            if (elem.children)
                result += print_tags(elem.tags);
            if (result != "")
                result += "<br/>";
        }
        
        if (feature.old && feature.new)
        {
            var result = "";
        
            var elem = feature.new;
            result += "<br/><span style=\"color:blue\">changed " + elem.type + " " + elem.id + "</span>";
            if (elem.tags && feature.old.tags)
                result += print_tags_diff(feature.old.tags, elem.tags);
            if (result != "")
                result += "<br/>";            
        }
        else if (feature.old)
        {
            var result = "";
        
            var elem = feature.old;
            result += "<br/><span style=\"color:red\">old " + elem.type + " " + elem.id + "</span>";
            if (elem.children)
                result += print_tags(elem.tags);
            if (result != "")
                result += "<br/>";
        }
        else if (feature.new)
        {
            var result = "";
        
            var elem = feature.new;
            result += "<br/><span style=\"color:green\">new " + elem.type + " " + elem.id + "</span>";
            if (elem.children)
                result += print_tags(elem.tags);
            if (result != "")
                result += "<br/>";
        }
        
        if (target && target.innerHTML)
            target.innerHTML += result;
    }
    
    
    var result = {
        print_single_elem: print_elem
    };
    return result;
}

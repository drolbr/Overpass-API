
function OsmToHtml(dom_element)
{
    function print_tags(tags, target)
    {
        for (var key in tags)
        {
            target.appendChild(document.createElement("br"));
            target.appendChild(document.createTextNode(key + " = " + tags[key]));
        }
    }

    
    function print_tags_diff(old_tags, new_tags, target)
    {
        for (key in old_tags)
        {
            if (!new_tags[key] || old_tags[key] != new_tags[key])
            {
                target.appendChild(document.createElement("br"));
                var span = document.createElement("span");
                span.setAttribute("style", "color:red");
                span.appendChild(document.createTextNode(
                    "- " + key + " = " + old_tags[key]));
                target.appendChild(span);
            }
        }
            
        for (key in new_tags)
        {
            if (!old_tags[key] || old_tags[key] != new_tags[key])
            {
                target.appendChild(document.createElement("br"));
                var span = document.createElement("span");
                span.setAttribute("style", "color:green");
                span.appendChild(document.createTextNode(
                    "+ " + key + " = " + new_tags[key]));
                target.appendChild(span);
            }
        }
            
        for (key in new_tags)
        {
            if (old_tags[key] && old_tags[key] == new_tags[key])
            {
                target.appendChild(document.createElement("br"));
                target.appendChild(document.createTextNode(
                    "\u00A0 " + key + " = " + new_tags[key]));
            }
        }
    }
    
    
    function print_elem(feature, target)
    {
        if (feature.keep)
        {
            var elem = feature.keep;
            target.appendChild(document.createElement("br"));
            target.appendChild(document.createTextNode(elem.type + " " + elem.id));
            if (elem.tags)
                print_tags(elem.tags, target);
            target.appendChild(document.createElement("br"));
        }
        
        if (feature.old && feature.new && feature.action != "delete")
        {
            var elem = feature.new;
            target.appendChild(document.createElement("br"));
            var span = document.createElement("span");
            span.setAttribute("style", "color:blue");
            span.appendChild(document.createTextNode(elem.type + " " + elem.id));
            target.appendChild(span);
            if (elem.tags && feature.old.tags)
                print_tags_diff(feature.old.tags, elem.tags, target);
            target.appendChild(document.createElement("br"));
        }
        else if (feature.old)
        {
            var elem = feature.old;
            target.appendChild(document.createElement("br"));
            var span = document.createElement("span");
            span.setAttribute("style", "color:red");
            span.appendChild(document.createTextNode(elem.type + " " + elem.id));
            target.appendChild(span);
            if (elem.tags)
                print_tags(elem.tags, target);
            target.appendChild(document.createElement("br"));
        }
        else if (feature.new)
        {
            var elem = feature.new;
            target.appendChild(document.createElement("br"));
            var span = document.createElement("span");
            span.setAttribute("style", "color:green");
            span.appendChild(document.createTextNode(elem.type + " " + elem.id));
            target.appendChild(span);
            if (elem.tags)
                print_tags(elem.tags, target);
            target.appendChild(document.createElement("br"));
        }
        
        return result;
    }
    
    
    var result = {
        print_single_elem: print_elem
    };
    return result;
}

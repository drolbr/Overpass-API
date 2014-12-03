
function OsmToOpenLayers(feature_layer_, osm_elements_)
{
    var feature_layer = feature_layer_;
    var osm_elements = osm_elements_;
    var bounds = false;
    
  
    function wgsPoint(lat, lon)
    {
        var web_mercator = new OpenLayers.LonLat(lon, lat).transform(new OpenLayers.Projection("EPSG:4326"), new OpenLayers.Projection("EPSG:900913"));
        return new OpenLayers.Geometry.Point(web_mercator.lon, web_mercator.lat);
    }
    
    
    //-------------------------------------------------------------------------

    
    function add_to_viewport(point)
    {
        if (!bounds || !bounds.extend)
	    bounds = new OpenLayers.Bounds(point.x, point.y, point.x, point.y);
	else
	    bounds.extend(point);
    }

    
    function add_elem_to_viewport(elem)
    {
        if (elem.type == "node")
	    add_to_viewport(wgsPoint(elem.geometry.y, elem.geometry.x));        
    }

        
    function add_feature_to_viewport_(index)
    {
        if (index >= 0 && index < osm_elements.length)
        {
            var elem = osm_elements[index];
      
            if (elem.old)
                add_elem_to_viewport(elem.old);
            if (elem.new)
                add_elem_to_viewport(elem.new);
            if (elem.keep)
                add_elem_to_viewport(elem.keep);
	}
    }
    
    
    //---------------------------------------------------------------------

    
    function show_partial_element(elem, color)
    {
        if (!elem.geometry || !elem.id)
            return;
            
        if (elem.type == "node")
        {
            var feature = new OpenLayers.Feature.Vector(
                wgsPoint(elem.geometry.y, elem.geometry.x),
		{ descr: "node " + elem.id },
		{ fillColor: color,
		  stroke: false,
		  pointRadius: 12 });
	    feature_layer.addFeatures([feature]);
        }
        else if (elem.type == "way")
        {
            for (var i = 0; i < elem.geometry.length; ++i)
            {
                var ol_points = [];
                for (var j = 0; j < elem.geometry[i].length; ++j)
                    ol_points.push(wgsPoint(elem.geometry[i][j].y, elem.geometry[i][j].x));
                var feature = new OpenLayers.Feature.Vector(
                    new OpenLayers.Geometry.LineString(ol_points),
                    { descr: "way " + elem.id },
                    { fill: false,
		      strokeWidth: 10,
		      strokeColor: color });
	        feature_layer.addFeatures([feature]);
            }
        }
    }
    
    
    function show_element(index)
    {
        if (index >= 0 && index < osm_elements.length)
        {
            var elem = osm_elements[index];
            if (elem.keep)
                show_partial_element(elem.keep, "#0000ff");
            else if (elem.old && elem.new && OsmXmlParser().equal_geometry(elem.old.geometry, elem.new.geometry))
                show_partial_element(elem.new, "#0000ff");
            else
            {
                if (elem.old)
                    show_partial_element(elem.old, "#ff0000");
                if (elem.new)
                    show_partial_element(elem.new, "#00ff00");
            }
        }
    }
    
    
    function show_single_element_(index)
    {
        feature_layer.removeAllFeatures();
	show_element(index);
    }
    
    
    function show_single_element_closure_(index)
    {
        var i = index;
        
        function handler(event)
        {
            show_single_element_(i);
        }
        
        return handler;
    }
    
    
    function show_all_elements_()
    {
        feature_layer.removeAllFeatures();
        for (var i = 0; i < osm_elements.length; ++i)
            show_element(i);
    }
    
    
    function get_bounds_()
    {
        return bounds;
    }
    
    
    var result = {
        add_feature_to_viewport: add_feature_to_viewport_,
	get_bounds: get_bounds_,
        show_single_element: show_single_element_,
	show_single_element_closure: show_single_element_closure_,
	show_all_elements: show_all_elements_
    };
    return result;
}

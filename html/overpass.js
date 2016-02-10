
      OpenLayers.Control.Click = OpenLayers.Class(OpenLayers.Control, {
	
        genericUrl: "",
	tolerance: 0.0,
	map: "",
      
	defaultHandlerOptions: {
	  'single': true,
	  'double': false,
	  'pixelTolerance': 0,
	  'stopSingle': false,
	  'stopDouble': false
	},
	
	initialize: function(url, tolerance, map) {
	  this.genericUrl = url;
	  this.tolerance = tolerance;
	  this.map = map;
	
	  this.handlerOptions = OpenLayers.Util.extend(
	    {}, this.defaultHandlerOptions
	  );
	  OpenLayers.Control.prototype.initialize.apply(
	    this, arguments
	  ); 
	  this.handler = new OpenLayers.Handler.Click(
	    this, {
	      'click': this.trigger
	    }, this.handlerOptions
	  );
	}, 
	
	trigger: function(evt) {
	  var lonlat = map.getLonLatFromPixel(evt.xy)
	      .transform(new OpenLayers.Projection("EPSG:900913"),
		         new OpenLayers.Projection("EPSG:4326"));
	      
	  var popup = new OpenLayers.Popup("location_info",
                   new OpenLayers.LonLat(lonlat.lon, lonlat.lat)
		       .transform(new OpenLayers.Projection("EPSG:4326"), new OpenLayers.Projection("EPSG:900913")),
		   null,
                   "Loading ...",
	           true);
	  
	  popup.contentSize = new OpenLayers.Size(400, 400);
	  popup.closeOnMove = true;
	  map.addPopup(popup);

	  var rel_tolerance = this.tolerance * map.getScale();
	  if (rel_tolerance > 0.01)
	    rel_tolerance = 0.01;

	  var request = OpenLayers.Request.GET({
	      url: this.genericUrl + "&bbox="
	          + (lonlat.lon - rel_tolerance) + "," + (lonlat.lat - rel_tolerance) + ","
	          + (lonlat.lon + rel_tolerance) + "," + (lonlat.lat + rel_tolerance),
	      async: false
          });
	  
	  map.removePopup(popup);
	  popup.contentHTML = request.responseText;
	  map.addPopup(popup);
	}
	
      });

// ----------------------------------------------------------------------------
      
      function setStatusText(text)
      {
	  var html_node = document.getElementById("statusline");
	  if (html_node != null)
	  {
            var div = html_node.firstChild;
            div.deleteData(0, div.nodeValue.length);
            div.appendData(text);
	  }
      }

      var zoom_valid = true;
      var load_counter = 0;      
      
      function make_features_added_closure(layer) {
	  var layer_ = layer;
          return function(evt) {
	      setStatusText("Displaying " + layer_.features.length + " features ...");
          };
      }

      ZoomLimitedBBOXStrategy = OpenLayers.Class(OpenLayers.Strategy.BBOX, {

          zoom_data_limit: 13,

          initialize: function(zoom_data_limit) {
              this.zoom_data_limit = zoom_data_limit;
          },

          update: function(options) {
	      this.ratio = this.layer.ratio;
              var mapBounds = this.getMapBounds();
              if (this.layer && this.layer.map && this.layer.map.getZoom() < this.zoom_data_limit) {
                  setStatusText("Please zoom in to view data.");
                  zoom_valid = false;

                  this.bounds = null;
              }
              else if (mapBounds !== null && ((options && options.force) ||
                                         this.invalidBounds(mapBounds))) {
                  ++load_counter;
                  setStatusText("Loading data ...");
                  zoom_valid = true;

                  this.calculateBounds(mapBounds);
                  this.resolution = this.layer.map.getResolution();
                  this.triggerRead(options);
              }
          },

          CLASS_NAME: "ZoomLimitedBBOXStrategy"
      });
      
      OSMTimeoutFormat = OpenLayers.Class(OpenLayers.Format.OSM, {
	
	  initialize: function(strategy) {
	      this.strategy = strategy;
	  },
	
	  read: function(doc)
          {
              if (typeof doc == "string") {
                  doc = OpenLayers.Format.XML.prototype.read.apply(this, [doc]);
              }
              
	      var feat_list = OpenLayers.Format.OSM.prototype.read.apply(this, [doc]);
	      
	      if (this.strategy)
	      {
		  var node_list = doc.getElementsByTagName("remark");
		  if (node_list.length > 0)
		  {
		      setStatusText("Please zoom in to view data.");
		      this.strategy.bounds = null;
		  }
	      }
	      
	      return feat_list;
	  },
	  
	  strategy: null,

          CLASS_NAME: "OSMTimeoutFormat"
      });

//-----------------------------------------------------------------------------
      
var OSMDiffFormat = OpenLayers.Class(OpenLayers.Format.OSM, {
      
    extent: {},
    
    setStatus: function(status) {},
                                     
    pushTextualResult: function(feature) {},
       
    initialize: function(options)
    {
        if (options && options.extent)
            this.extent = options.extent;
        if (options && options.setStatus)
            this.setStatus = options.setStatus;
        if (options && options.pushTextualResult)
            this.pushTextualResult = options.pushTextualResult;
    },
        
    read: function(doc)
    {
        this.setStatus("Processing data");

        if (typeof doc == "string")
            doc = OpenLayers.Format.XML.prototype.read.apply(this, [doc]);
  
        var feat_list = [];
  
        var relations = this.getRelations(doc);
        for (var relation_id in relations)
            feat_list.push(relations[relation_id]);
  
        var ways = this.getWays(doc);
        for (var way_id in ways)
            feat_list.push(ways[way_id]);
  
        var nodes = this.getNodes(doc);
        for (var node_id in nodes)
            feat_list.push(nodes[node_id]);
  
        this.setStatus("Ready");
        return feat_list;
    },

    isInExtent: function(node)
    {
        if (!this.extent)
          return true;
        else if (node.getAttribute("lon") && node.getAttribute("lat"))
        {
            var geom = new OpenLayers.Geometry.Point(
                node.getAttribute("lon"), node.getAttribute("lat"));
            return (geom.x <= this.extent.right && geom.x >= this.extent.left &&
                geom.y <= this.extent.top && geom.y >= this.extent.bottom);
        }
        else
          return false;
    },
    
    stateOf: function(element)
    {
        var state = {};
        if (element.parentNode.nodeName == "old")
            state = { state: "old" };
        else
            state = { state: "new" };
        
        return state;
    },
    
    pointGeomFromNode: function(node)
    {
        var geom = new OpenLayers.Geometry.Point(
            node.getAttribute("lon"), node.getAttribute("lat"));
        if (this.internalProjection && this.externalProjection)
            geom.transform(this.externalProjection, this.internalProjection);
        return geom;
    },
    
    pointListFromWay: function(node_list)
    {
        var lower = 0;
        while (lower < node_list.length && !this.isInExtent(node_list[lower]))
            ++lower;
        if (lower > 0 && node_list[lower-1].getAttribute("lat") && node_list[lower-1].getAttribute("lon"))
            --lower;
            
        var upper = node_list.length;
        while (upper > 0 && !this.isInExtent(node_list[upper-1]))
            --upper;
        if (upper < node_list.length
                && node_list[upper].getAttribute("lat") && node_list[upper].getAttribute("lon"))
            ++upper;
        
        if (upper < lower)
            return new Array();
        
        var point_list = new Array(upper - lower);
        var pos = 0;
        for (var j = lower; j < upper; ++j)
        {
            if (node_list[j].getAttribute("lat") && node_list[j].getAttribute("lon"))
                point_list[pos++] = this.pointGeomFromNode(node_list[j]);
        }
        point_list = point_list.slice(0, pos);
            
        return point_list;
    },
          
    getNodes: function(doc)
    {
        var node_list = doc.getElementsByTagName("node");
        var nodes = {};
        for (var i = 0; i < node_list.length; ++i)
        {
            var node = node_list[i];
            var id = node.getAttribute("id");

            var state = this.stateOf(node);                
            var feat = new OpenLayers.Feature.Vector(this.pointGeomFromNode(node), state);
            feat.tags = this.getTags(node);
            feat.osm_id = parseInt(id);
            feat.osm_version = parseInt(node.getAttribute("version"));
            feat.type = "node";
            feat.fid = "node." + feat.osm_id;
            feat.geometry.osm_id = feat.osm_id;

            if (this.isInExtent(node))
            {
                nodes[id + "." + state.state] = feat;
                this.pushTextualResult(feat);
            }
        }
        return nodes;
    },
          
    getWays: function(doc) {
        var way_list = doc.getElementsByTagName("way");
        var ways = {};
        for (var i = 0; i < way_list.length; ++i)
        {
            var way = way_list[i];
            var id = way.getAttribute("id");
            
            var state = this.stateOf(way);
            var way_nodes = this.pointListFromWay(way.getElementsByTagName("nd"));
            var feat = new OpenLayers.Feature.Vector(new OpenLayers.Geometry.LineString(way_nodes), state);
            feat.tags = this.getTags(way);
            feat.osm_id = parseInt(id);
            feat.osm_version = parseInt(way.getAttribute("version"));
            feat.type = "way";
            feat.fid = "way." + feat.osm_id;

            if (way_nodes.length >= 2)
            {
                ways[id + "." + state.state] = feat;
                this.pushTextualResult(feat);
            }
        }
        return ways;
    },

    getRelations: function(doc) {
        var relation_list = doc.getElementsByTagName("relation");
        var return_relations = {};
        for (var i = 0; i < relation_list.length; ++i)
        {
            var relation = relation_list[i];
            var id = relation.getAttribute("id");
            
            var member_list = relation.getElementsByTagName("member");
            var geom = new OpenLayers.Geometry.Collection();
            for (var j = 0; j < member_list.length; ++j)
            {
                if (member_list[j].getAttribute("type") == "node" && this.isInExtent(member_list[j]))
                    geom.addComponent(this.pointGeomFromNode(member_list[j]));
                else if (member_list[j].getAttribute("type") == "way")
                {
                    var way_nodes = this.pointListFromWay(member_list[j].getElementsByTagName("nd"));
                    if (way_nodes.length >= 2)
                        geom.addComponent(new OpenLayers.Geometry.LineString(way_nodes));
                }
            }
            
            var state = this.stateOf(relation);
            var feat = new OpenLayers.Feature.Vector(geom, state);
            feat.tags = this.getTags(relation);
            feat.osm_id = parseInt(id);
            feat.osm_version = parseInt(relation.getAttribute("version"));
            feat.type = "relation";
            feat.fid = "relation." + feat.osm_id;

            if (geom.components.length > 0)
            {
                return_relations[id + "." + state.state] = feat;
                this.pushTextualResult(feat);
            }
        }
        return return_relations;
    },

    strategy: null,

    CLASS_NAME: "OSMDiffFormat"
});

//-----------------------------------------------------------------------------

      function make_large_layer(data_url, color, zoom) {

          var styleMap = new OpenLayers.StyleMap({
              strokeColor: color,
              strokeOpacity: 0.5,
              strokeWidth: 6,
              pointRadius: 10,
              fillColor: color,
              fillOpacity: 0.25
          });
	  
	  var strategy = new ZoomLimitedBBOXStrategy(zoom);
          var layer = new OpenLayers.Layer.Vector(color, {
              strategies: [strategy],
              protocol: new OpenLayers.Protocol.HTTP({
                  url: data_url,
                  format: new OSMTimeoutFormat(strategy)
              }),
              styleMap: styleMap,
              projection: new OpenLayers.Projection("EPSG:4326"),
              ratio: 1.0
          });

          layer.events.register("featuresadded", layer, make_features_added_closure(layer));

          return layer;
      }

      function make_layer(data_url, color) {
	  return make_large_layer(data_url, color, 8);
      }
      
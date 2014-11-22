
function ChangesetContentRequest()
{
    function process_changeset(changeset, status_bar, on_completion)
    {
        var id = "";
        if (changeset.attributes && changeset.attributes.getNamedItem("id"))
            id = changeset.attributes.getNamedItem("id").value;
                
        var min_lat = "";
        if (changeset.attributes && changeset.attributes.getNamedItem("min_lat"))
            min_lat = changeset.attributes.getNamedItem("min_lat").value;

        var min_lon = "";
        if (changeset.attributes && changeset.attributes.getNamedItem("min_lon"))
            min_lon = changeset.attributes.getNamedItem("min_lon").value;

        var max_lat = "";
        if (changeset.attributes && changeset.attributes.getNamedItem("max_lat"))
            max_lat = changeset.attributes.getNamedItem("max_lat").value;

        var max_lon = "";
        if (changeset.attributes && changeset.attributes.getNamedItem("max_lon"))
            max_lon = changeset.attributes.getNamedItem("max_lon").value;

        var created_at = "";
        if (changeset.attributes && changeset.attributes.getNamedItem("created_at"))
            created_at = changeset.attributes.getNamedItem("created_at").value;

        var closed_at = "";
        if (changeset.attributes && changeset.attributes.getNamedItem("closed_at"))
            closed_at = changeset.attributes.getNamedItem("closed_at").value;

        if (id != "" && min_lat != "" && min_lon != "" && max_lat != "" && max_lon != "")
        {
            var date_delta = "\"" + created_at +"\",\""+ closed_at + "\"";
            var bbox = "(" + min_lat + "," + min_lon + "," + max_lat + "," + max_lon + ")";
            var url = "/api/interpreter?data=[adiff:" + date_delta + "];"
                + "(node(changed:" + date_delta + ")" + bbox + ";"
                + "way(changed:" + date_delta + ")" + bbox + ";"
                + "rel(changed:" + date_delta + ")" + bbox + ";);out meta geom;";
            if (status_bar && status_bar.innerHTML)
                status_bar.innerHTML = "requested changeset " + id;
            httpXmlWrapper(url, on_completion);
        }
    }
    
    return process_changeset;
}

const $ = go.GraphObject.make;
export const diagram =
  $(go.Diagram, "diagramDiv",
    {
      "undoManager.isEnabled": true,
      initialAutoScale: go.Diagram.Uniform,  // an initial automatic zoom-to-fit
      contentAlignment: go.Spot.Center,
      layout:
         $(go.ForceDirectedLayout,
        // $(go.GridLayout)
        // $(go.CircularLayout)
          { defaultSpringLength: 30, defaultElectricalCharge: 100 }),
      "SelectionMoved": function (e) { e.diagram.layout.invalidateLayout(); }
    }
  );

function makeButton(text, action) {
  return $("ContextMenuButton",
    $(go.TextBlock, "default",
      {
        margin: 3,
        stroke: "rgb(63, 62, 62)",
        font: "12px sans-serif"
      }, text),
    { click: action });
}

const nodeContextMenu =
  $("ContextMenu",
    makeButton("Properties",
      function (e, obj) {  // OBJ is this Button
        var contextmenu = obj.part;  // the Button is in the context menu Adornment
        var part = contextmenu.adornedPart;  // the adornedPart is the Part that the context menu adorns
        // now can do something with PART, or with its data, or with the Adornment (the context menu)
        alert(nodeProperties(part.data));

      }))




// Node
diagram.nodeTemplate =
  $(go.Node, "Auto",
    { locationSpot: go.Spot.Center, background: "lightblue", visible: true },
    new go.Binding("location", "loc", go.Point.parse),
    new go.Binding("visible", "visible"),
    $(go.Shape,
      "RoundedRectangle",
      {
        fill: "lightblue",
        stroke: "gray",
        strokeWidth: 2
      }),
    $(go.TextBlock,
      "default",
      {
        margin: 12,
        stroke: "gray",
        font: "bold 16px sans-serif"
      },
      new go.Binding("text", "name")),
    {
      selectionChanged: function (part) {
        var shape = part.elt(0);
        shape.fill = part.isSelected ? "lightyellow" : "lightblue";
      },
      toolTip:
        $("ToolTip",
          $(go.TextBlock, {
            margin: 3,
            stroke: "gray",
            font: "12px sans-serif"
          },
            new go.Binding("text", "", nodeProperties))
        ),
      contextMenu: nodeContextMenu
    }
  );

function nodeProperties(d) {
  return "Methods: " + (d.data.methods ? Object.keys(d.data.methods).length : "-") +
    "\nFields: " + (d.data.fields ? Object.keys(d.data.fields).length : "-") +
    "\nBases: " + (d.data.bases ? Object.keys(d.data.bases).length : "-") +
    "\nFriends: " + (d.data.friends ? Object.keys(d.data.friends).length : "-");
}


// Group
diagram.groupTemplate = $(go.Group, "Vertical",
  $(go.Panel, "Auto",
    $(go.Shape, "RoundedRectangle",  // surrounds the Placeholder
      {
        // parameter1: 14,
        fill: "rgba(128,128,128,0.33)",
        visible: true
      },
      new go.Binding("visible", "visible"),
      new go.Binding("fill", "fill"),
    ),
    $(go.Placeholder,
      { padding: 7 })
  ),
  $(go.TextBlock,         // group title
    {
      alignment: go.Spot.Default,
      margin: 10,
      font: "Bold 12pt sans-serif",
      visible: true
    },
    new go.Binding("text", "name")),
  new go.Binding("visible", "visible")
);


// Link
diagram.linkTemplate =
  $(go.Link,
    { toShortLength: 6, toEndSegmentLength: 20, curve: go.Link.Bezier },
    new go.Binding("visible", "visibleLink"),
    $(go.Shape,                                           // link
      {
        strokeWidth: 0.6,
        stroke: "#555555"
      },
      new go.Binding("strokeWidth", "thick")),
    $(go.Shape,                                           // arrowhead
      {
        toArrow: "standard",
        stroke: "#555555",
        scale: 0.9
      }),

    $(go.Panel, "Auto",
      $(go.Shape,  // the label background, which becomes transparent around the edges
        {
          fill: $(go.Brush, "Radial", { 0: "rgb(215, 220, 223)", 0.3: "rgb(215, 220, 223)", 1: "rgba(215, 220, 223, 0)" }),
          stroke: null
        }),
      $(go.TextBlock,  // the label text
        {
          textAlign: "center",
          font: " 10pt helvetica, arial, sans-serif",
          stroke: "#777",
          margin: 4,
          visible: true
        },
        new go.Binding("text", "weight"),
        new go.Binding("visible", "visibleWeight"))
    )
  );

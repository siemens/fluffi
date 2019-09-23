§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Junes Najah, Thomas Riedmaier, Abian Blome
§§*/
§§
§§function getPopulationGraph(projId) {
§§    var changeUrl = "/projects/" + projId + "/testcaseGraph";
§§    nodes = [];
§§    edges = [];
§§    
§§    $.getJSON(changeUrl, function(data) {
§§        $.each(data["nodes"], function(index) {
§§            var myNodes = data["nodes"][index];
§§            var nodeName = myNodes["realId"];
§§            var myType = myNodes["type"] ? myNodes["type"] : "FOOTPRINT";
§§            
§§            if(myNodes["crashFootprint"])
§§                nodeName = myNodes["crashFootprint"];
§§            else if(myNodes["niceNameTC"])
§§                nodeName = myNodes["niceNameTC"];
§§            else if(myNodes["niceNameMI"])
§§                nodeName = myNodes["niceNameMI"];
§§
§§            nodes.push( {data: {id: myNodes["cyId"], nodeName: nodeName, type: myType, downloadId: myNodes["realId"] } });
§§        });
§§        $.each(data["edges"], function(index) {
§§            edges.push( {data: {source: data["edges"][index]["parent"], target: data["edges"][index]["child"], label: data["edges"][index]["label"]} });
§§        });
§§          
§§        var cy = window.cy = cytoscape({
§§            container: document.getElementById('cy'),
§§            boxSelectionEnabled: false,
§§            autounselectify: true,
§§            layout: {
§§                name: 'dagre'
§§            },
§§            style: [ {
§§                selector: 'node',
§§                style: {
§§                    'content': 'data(nodeName)',
§§                    'text-opacity': 0.5,
§§                    'text-valign': 'center',
§§                    'text-halign': 'right',
§§                    'background-color': '#11479e'
§§                }},
§§                {
§§                selector: 'edge',
§§                style: {
§§                    'curve-style': 'bezier',
§§                    'width': 4,
§§                    'target-arrow-shape': 'triangle',
§§                    'line-color': '#9dbaea',
§§                    'target-arrow-color': '#9dbaea',
§§                    'label': function(element){
§§                        if(element.data('label') > 0){
§§                            return element.data('label');
§§                        }                        
§§                    }
§§                }
§§                } ],
§§            elements: {
§§                nodes: nodes,
§§                edges: edges
§§            },
§§        });
§§
§§        cy.filter('node[type="5"]').style('background-color', "grey");
§§        cy.filter('node[type="FOOTPRINT"]').style('background-color', "orange");    
§§
§§        cy.on('tap', 'node', function(evt){
§§            var nodeData = evt.target.data();
§§            
§§            if(nodeData["type"] == "FOOTPRINT"){
§§                window.location = '/projects/' + projId + '/crashesorvio/download/' + nodeData["id"]
§§            } else {
§§                window.location = '/projects/' + projId + '/getTestcase/' + nodeData["downloadId"];                
§§            }        
§§        });
§§
§§        cy.on('tap', 'node', function(evt){
§§            var nodeData = evt.target.data();
§§            
§§            if(nodeData["type"] == "FOOTPRINT"){
§§                window.location = '/projects/' + projId + '/crashesorvio/download/' + nodeData["id"]
§§            } else {
§§                window.location = '/projects/' + projId + '/getTestcase/' + nodeData["downloadId"];
§§            }
§§        });
§§        cy.on('cxttap', 'node', function(evt){
§§            var nodeData = evt.target;
§§            nodeData.style('display', "none");
§§        });
§§    });
§§}

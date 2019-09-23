/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Junes Najah, Thomas Riedmaier
*/

§§var clearId = 0;
§§var rowCount = 0;
§§var lastTimeout = 0;
§§
§§$(function () {
§§    autoSwap();
§§});
§§
§§function Swap(){
§§    var table = document.getElementById("fuzzjobTable");
§§    var row;
§§    row = table.rows[rowCount];
§§    if (row) { row.style.backgroundColor = "#ffffff"; }
§§    rowCount++;
§§
§§    if(rowCount > (table.rows.length-1)){
§§      rowCount = 1;
§§    }
§§    
§§    row = table.rows[rowCount];
§§    if (row) {
§§        row.style.backgroundColor = "#BDBDBD";
§§        var projectID = row.cells[0].innerText;
§§        var res = "http://mon.fluffi/dashboard/script/scripted.js?fuzzjobname=" + projectID + "&from=now-15m&to=now&orgId=1&kiosk";
§§        $("#icontainer").empty();
§§        $("#icontainer").append("<iframe width='100%' src='" + res + "' />");
§§    }
§§}
§§ 
§§function autoSwap() {
§§    Swap();
§§    clearId = setInterval(function(){
§§        Swap();
§§    }, 60000);
§§}
§§
§§function showProjectGraph(projName) {
§§    clearInterval(clearId);
§§    var Url = "http://mon.fluffi/dashboard/script/scripted.js?fuzzjobname=" + projName + "&from=now-15m&to=now&orgId=1&kiosk";
§§    var res = Url.replace(/'/g, "%27");
§§    $("#icontainer").empty();
§§    $("#icontainer").append("<iframe width='100%' src='" + res + "' />");
§§    var table = document.getElementById("fuzzjobTable");
§§    var row;
§§    row = table.rows[rowCount];
§§    if (row) {
§§        row.style.backgroundColor = "#ffffff";
§§        for (var i = 0, row; row = table.rows[i]; i++) {
§§            //iterate through rows
§§            //rows would be accessed using the "row" variable assigned in the for loop
§§            if(row.cells[0].innerText==projName){
§§                row.style.backgroundColor = "#BDBDBD";
§§                rowCount = i;
§§            }
§§        }
§§    }
§§    
§§    clearTimeout(lastTimeout);
§§    lastTimeout = setTimeout(function(){ autoSwap(); }, 60000);
}

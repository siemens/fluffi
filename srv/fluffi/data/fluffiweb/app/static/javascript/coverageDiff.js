/*
Copyright 2017-2020 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

Author(s): Junes Najah
*/

const MODULES_CONTAINER_ID = "#ModulesContainer";


function getTableRow(row){
    return "<tr><td>" + row.tcName + ": " + row.tcBlocks + "</td><td>" + row.overlap + "</td><td>" + row.parentName + ": " + row.parentBlocks + "</td></tr>";
}


function getModuleTable(moduleName, row){
    var htmlString = "<h3>Module " + moduleName + "</h3><table class='table table-bordered table-hover'><thead><tr>\
                    <th>Testcase: Covered Blocks</th><th>Overlap</th><th>Parent: Covered Blocks</th></tr></thead><tbody>";    
    htmlString += getTableRow(row);
    htmlString += "</tbody></table><br>";  
    
    return htmlString;
}


function loadCoverageDiff(data) {    

    $.ajax({
        url: "/projects/coverageDiff",
        type: 'POST',
        data: JSON.stringify(data),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        success: function(response) {            
            if (response.status === "OK"){                
                response.modules.forEach(function(module){  
                    $(MODULES_CONTAINER_ID).append(getModuleTable(module.moduleName, module.data));                                                            
                });         
            } else {
                console.log(response["message"]);
            }
        },
        error: function(response) {
            console.log(response);            
        }
    });  
}


function getParameterByName(name, url) {
    if (!url) url = window.location.href;
    name = name.replace(/[\[\]]/g, '\\$&');
    var regex = new RegExp('[?&]' + name + '(=([^&#]*)|&|#|$)'),
        results = regex.exec(url);
    if (!results) return null;
    if (!results[2]) return '';
    return decodeURIComponent(results[2].replace(/\+/g, ' '));
}


$(document).ready(function() {
    const projId = getParameterByName('projectID');
    const testcaseId = getParameterByName('testcaseID');

    if (projId !== undefined && testcaseId !== undefined){
        const data = {projId, testcaseId};
        loadCoverageDiff(data);   
    } else {
        console.log("Project Id or testcase Id is undefined");
    }   
});
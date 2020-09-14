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

const TC_CONTAINER_ID = "#testcaseCovContainer";
const OVERLAP_CONTAINER_ID = "#overlapCoverageContainer";
const PARENT_CONTAINER_ID = "#parentCoverageContainer";


function loadCoverageDiff(projId, testcaseId, loopIndex) {
    const data = {projId, testcaseId};

    $.ajax({
        url: "/projects/coverageDiff",
        type: 'POST',
        data: JSON.stringify(data),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        success: function(response) {            
            if (response.status === "OK"){                
                
                if ($(TC_CONTAINER_ID + loopIndex).children().length == 1) {
                    response.coverageTestcase.forEach(function(elem){  
                        var style = response.overlapModuleNames.includes(elem.moduleName) ? "style='color:red;'" : "";
                        $(TC_CONTAINER_ID + loopIndex).append("<p " + style + ">" + elem.moduleName + ": " + elem.coveredBlocks + "</p>");                                                            
                    });  
                }                

                if ($(PARENT_CONTAINER_ID + loopIndex).children().length == 0) {
                    $(PARENT_CONTAINER_ID + loopIndex).append("<h4>Covered Blocks of parent " + response.parentNiceName + "</h4>");                    
                    response.coverageParent.forEach(function(elem){
                        var style = response.overlapModuleNames.includes(elem.moduleName) ? "style='color:red;'" : "";
                        $(PARENT_CONTAINER_ID + loopIndex).append("<p " + style + ">" + elem.moduleName + ": " + elem.coveredBlocks + "</p>");
                    });
                }  

                $("#loader" + loopIndex).css('display', 'none');
                $(TC_CONTAINER_ID + loopIndex).css('visibility', 'visible');          
                $(OVERLAP_CONTAINER_ID + loopIndex).css('visibility', 'visible');          
                $(PARENT_CONTAINER_ID + loopIndex).css('visibility', 'visible');    
            } else {
                console.log(response["message"]);
            }
        },
        error: function(response) {
            console.log(response);            
        }
    });  
}
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

Author(s): Pascal Eckmann
*/

var MARGIN = 5;

function loadHexdump(projId, testcaseID, loopIndex, offset=0, currentPage=1, init=true){
    var URL = "/hexdump/" + projId + "/" + testcaseID + "/" + offset;
    var data = {};
    var hexData = [];
    var pageCount = 1;
    var linkElem = "";
    var navigateFuncStr = "";

    $(".modal-loader").css("display", "inline");
    $("#buildHexdump" + loopIndex).css("visibility", "hidden");

    $.ajax({
        url: URL,
        type: 'GET',
        data: JSON.stringify(data),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        success: function(response) {
            if(response["status"] == "OK"){
                hexData = response["hex"];
                decodedData = response["decoded"];
                pageCount = response["pageCount"];
                // build hex elements
                $("#buildHexdump" + loopIndex).empty();
                var hexTableHtml = $("<table id=\"hexTable\"><tr class=\"by\"><td class=\"br\">Offset(h)</td><td>00</td><td>01</td><td>02</td><td>03</td><td>04</td><td>05</td><td>06</td><td>07</td><td>08</td><td>09</td><td>0A</td><td>0B</td><td>0C</td><td>0D</td><td>0E</td><td>0F</td><td class=\"bp\" colspan=\"16\">Decoded text</td></tr></table>");
                $("#buildHexdump" + loopIndex).append(hexTableHtml);

                $.each(hexData, function(key, row){
                    var hexTableRow = $("<tr></tr>");
                    $(hexTableHtml).append(hexTableRow);
                    var offsetNum = ((key + (currentPage - 1) * 20) * 16).toString(16);
                    var offsetZeroLength = 8 - offsetNum.length;
                    for (var z = 0; z < offsetZeroLength; z++) {
                        offsetNum = "0" + offsetNum;
                    };
                    $(hexTableRow).append("<td class=\"br\"> " + offsetNum + "</td>");
                    var hexTableData = "";
                    var hexTableText = "";
                    $.each(row, function(i, value){
                        hexTableData = hexTableData.concat("<td>" + value + "</td>");
                        if (i==0){
                            hexTableText = hexTableText.concat("<td class=\"bp\">");
                        }else{
                            hexTableText = hexTableText.concat("<td>");
                        }
                        hexTableText = hexTableText.concat(decodedData[key][i] + "</td>");
                    });
                    $(hexTableRow).append(hexTableData);
                    $(hexTableRow).append(hexTableText);
                });

                if(init){
                    $("#buildLinks" + loopIndex).empty()
                    $("#buildLinks" + loopIndex).append("<li onclick='prevHex(" + loopIndex + ")' class='leftArrow'><a href='#' aria-label='Previous'><span aria-hidden='true'>&laquo;</span></a></li>");
                    for (var index = 1; index <= pageCount; index++) {
                        navigateFuncStr = "navigateHex(" + projId + ", " + testcaseID + ", " + loopIndex + ", " + index + ")";
                        var offsetNum = (((index - 1) * 20) * 16).toString(16);
                        if (index == currentPage){
                            linkElem = "<li id='" + loopIndex + "link" + index + "' class='active'><a href='#' onclick='" + navigateFuncStr + "'>" + offsetNum + "</a></li>";
                        }
                        else {
                            var style = index > 5 && index < pageCount ? "style='display:none'" : "";
                            linkElem = "<li id='" + loopIndex + "link" + index + "' " + style + "><a href='#' onclick='" + navigateFuncStr + "'>" + offsetNum + "</a></li>";
                        }

                        $("#buildLinks" + loopIndex).append(linkElem);

                        if(index == 1 && pageCount > (MARGIN + 1)) {
                            $("#buildLinks" + loopIndex).append("<li class='startDots' style='display:none;'><a href='#'>...</a></li>");
                        }
                        if(index == (pageCount-1) && pageCount > (MARGIN + 1)){
                            var liTagEndDots = pageCount <= MARGIN ? "<li class='endDots' style='display:none;'><a href='#'>...</a></li>" : "<li class='endDots'><a href='#'>...</a></li>";
                            $("#buildLinks" + loopIndex).append(liTagEndDots);
                        }
                    }
                    $("#buildLinks" + loopIndex).append("<li onclick='nextHex(" + loopIndex + ")' class='rightArrow'><a href='#' aria-label='Next'><span aria-hidden='true'>&raquo;</span></a></li>");
                }
                else if(pageCount > (MARGIN + 1)) {
                    updatePageLinksHex(loopIndex);
                }
                $(".modal-loader").css("display", "none");
                $("#buildHexdump" + loopIndex).css("visibility", "visible");
            }else{
                $(".modal-loader").css("display", "none");
                $("#buildHexdump" + loopIndex).css("visibility", "visible");
                $("#buildHexdump" + loopIndex).empty();
                $("#buildHexdump" + loopIndex).append("Something went wrong!");
            };
        },
        error: function(response) {
            $(".modal-loader").css("display", "none");
            $("#buildHexdump" + loopIndex).css("visibility", "visible");
            $("#buildHexdump" + loopIndex).empty();
            $("#buildHexdump" + loopIndex).append("Something went wrong!");
        }
    });    
}

function jumpToOffset(projId, testcaseID, loopIndex, offsetNum){
    if(event.key === 'Enter') {
        var page = Math.floor(parseInt("0x" + offsetNum.value) / 320) + 1;
        navigateHex(projId, testcaseID, loopIndex, page)
    }
}

function navigateHex(projId, testcaseID, loopIndex, currentPage){
    var offset = (currentPage-1) * 320;

    loadHexdump(projId, testcaseID, loopIndex, offset, currentPage, false);
    $("#buildLinks" + loopIndex).children().removeClass("active");
    $("#" + loopIndex + "link" + currentPage).addClass("active");
}

function nextHex(loopIndex){
    var links = $("#buildLinks" + loopIndex).children();  
    for (var index = 1; index < links.length-1; index++) {
        var nextIndex = links.eq(index+1).hasClass("startDots") || links.eq(index+1).hasClass("endDots") ? index + 2 : index + 1;        
        if(links.eq(index).hasClass("active") && links.eq(nextIndex).length){                    
            links.eq(nextIndex).children().first().click();    
            break;                
        }               
    }    
}

function prevHex(loopIndex){
    var links = $("#buildLinks" + loopIndex).children();        
    for (var index = 1; index < links.length-1; index++) {
        var nextIndex = links.eq(index-1).hasClass("endDots") || links.eq(index-1).hasClass("startDots") ? index - 2 : index - 1;
        if(links.eq(index).hasClass("active") && links.eq(nextIndex).length){             
            links.eq(nextIndex).children().first().click();    
            break;                
        }        
    } 
}

function updatePageLinksHex(loopIndex){
    var links = $("#buildLinks" + loopIndex).children();
    var indexOfActiveLink = null;
    var indexOfRightArrowLink = null;
    var indexOfLeftArrowLink = null;
    var endIndex = links.length-2; 

    links.each(function(i){
        if ($(this).hasClass("active"))
            indexOfActiveLink = i;
        else if ($(this).hasClass("rightArrow"))
            indexOfRightArrowLink = i;
        else if ($(this).hasClass("leftArrow"))
            indexOfLeftArrowLink = i;
    });

    // Used for distance from indexOfActiveLink to last element
    var distance = (links.length-2) - indexOfActiveLink;

    //disable arrow buttons if there are no more pages
    if (indexOfActiveLink !== null && indexOfRightArrowLink !== null && indexOfLeftArrowLink !== null) {
        if (links.eq(indexOfActiveLink+1).hasClass("rightArrow")) {
            links.eq(indexOfActiveLink+1).addClass("disabled");
        } else {
            links.eq(indexOfRightArrowLink).removeClass("disabled");
        }

        if (links.eq(indexOfActiveLink-1).hasClass("leftArrow")) {
            links.eq(indexOfActiveLink-1).addClass("disabled");
        } else {
            links.eq(indexOfLeftArrowLink).removeClass("disabled");
        }
    }    

    // Ending of pages  
    if (distance >= 0 && distance <= MARGIN) {
        // Hide endDots  
        // Show every page until links.length-2-MARGIN        
        // Hide every page until beginning                               
        for (var index = endIndex; index > 1; index--) { 
            if (links.eq(index).hasClass("endDots")) { links.eq(index).hide(); } 
            else if (links.eq(index).hasClass("startDots")) { links.eq(index).show(); }
            else if (index >= (endIndex-MARGIN)) { links.eq(index).show(); }
            else if (index < (endIndex-MARGIN)) { links.eq(index).hide(); }
        }
        if (!links.eq(indexOfActiveLink-1).hasClass("endDots")) { links.eq(indexOfActiveLink-1).show(); }        
        links.eq(indexOfActiveLink-2).show();
    } 
    // Beginning of pages
    else if (indexOfActiveLink <= MARGIN) {
        // Hide startDots
        // Show every page until MARGIN+1  
        // Hide every page after MARGIN+1  
        for (var index = 1; index < (links.length-2); index++) { 
            if (links.eq(index).hasClass("startDots")) { links.eq(index).hide(); } 
            else if (links.eq(index).hasClass("endDots")) { links.eq(index).show(); } 
            else if (index <= MARGIN+1) { links.eq(index).show(); }
            else if (index > MARGIN+1) { links.eq(index).hide(); }
        }        
        if (!links.eq(indexOfActiveLink+1).hasClass("startDots")) { links.eq(indexOfActiveLink+1).show(); }
        links.eq(indexOfActiveLink+2).show();
    } 
    // Middle pages
    else {
        links.eq(indexOfActiveLink+1).show();      
        links.eq(indexOfActiveLink+2).show();     
        for (var i = indexOfActiveLink+3; i < links.length-2; i++) {        
            if (links.eq(i).hasClass("endDots")) {
                links.eq(i).show();
                break;
            } else {
                links.eq(i).hide();
            }            
        }

        links.eq(indexOfActiveLink-1).show();                           
        links.eq(indexOfActiveLink-2).show(); 
        for (var i = indexOfActiveLink-3; i > 1; i--) {
            if (links.eq(i).hasClass("startDots")) {
                links.eq(i).show();
                break;
            } else {
                links.eq(i).hide();
            }            
        }
    }        
}

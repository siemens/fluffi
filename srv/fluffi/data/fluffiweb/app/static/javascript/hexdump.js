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

function loadHexdump(projId, testcaseID, loopIndex, mode="single", offset=0, currentPage=1, init=true, marker=-1){
    var URL = "/hexdump/" + projId + "/" + testcaseID + "/" + offset + "/0";
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
                hexData = response["hexT"];
                decodedData = response["decodedT"];
                pageCount = response["pageCount"];
                parent = response["parent"];

                // build hex elements
                $("#buildHexdump" + loopIndex).empty();
                $("#buildHexdump" + loopIndex).scrollTop(0);
                var hexTableHtml = $("<table id=\"hexTable\"><thead><tr class=\"by\"><td colspan=\"8\" class=\"br\">Offset(h)</td><td colspan=\"2\">00</td><td colspan=\"2\">01</td><td colspan=\"2\">02</td><td colspan=\"2\">03</td><td colspan=\"2\">04</td><td colspan=\"2\">05</td><td colspan=\"2\">06</td><td colspan=\"2\">07</td><td colspan=\"2\">08</td><td colspan=\"2\">09</td><td colspan=\"2\">0A</td><td colspan=\"2\">0B</td><td colspan=\"2\">0C</td><td colspan=\"2\">0D</td><td colspan=\"2\">0E</td><td colspan=\"2\">0F</td><td class=\"bp\" colspan=\"18\">Decoded text</td></tr></thead></table>");
                $("#buildHexdump" + loopIndex).append(hexTableHtml);
                var hexTableBody = $("<tbody></tbody>");
                $(hexTableHtml).append(hexTableBody);
                $.each(hexData, function(key, row){
                    var hexTableRow = $("<tr></tr>");
                    $(hexTableBody).append(hexTableRow);
                    var offsetNum = ((key + (currentPage - 1) * 60) * 16).toString(16);
                    var offsetZ = offsetNum
                    var offsetZeroLength = 8 - offsetZ.length;
                    for (var z = 0; z < offsetZeroLength; z++) {
                        offsetZ = "0" + offsetZ;
                    };
                    if(marker == offsetNum){
                        var markerElement = "<td id=\"markerElement\" colspan=\"8\" class=\"bx\"> " + offsetZ + "</td>"
                        $(hexTableRow).append(markerElement);
                        document.getElementById("markerElement").scrollIntoView();
                    }else{
                        $(hexTableRow).append("<td colspan=\"8\" class=\"br\"> " + offsetZ + "</td>");
                    }
                    var hexTableData = "";
                    var hexTableText = "";
                    $.each(row, function(i, value){
                        hexTableData = hexTableData.concat("<td colspan=\"2\">" + value + "</td>");
                        if (i==0){
                            hexTableText = hexTableText.concat("<td colspan=\"1\" class=\"bp\"></td><td colspan=\"1\">");
                        }else{
                            hexTableText = hexTableText.concat("<td colspan=\"1\">");
                        }
                        hexTableText = hexTableText.concat(decodedData[key][i] + "</td>");
                    });
                    hexTableText = hexTableText.concat("<td colspan=\"1\"></td>");
                    $(hexTableRow).append(hexTableData);
                    $(hexTableRow).append(hexTableText);
                });

                if(init){
                    $("#buildLinks" + loopIndex).empty();
                    $("#compare").remove();
                    $("#buildLinks" + loopIndex).append("<li onclick='prevHex(" + loopIndex + ")' class='leftArrow'><a href='#' aria-label='Previous'><span aria-hidden='true'>&laquo;</span></a></li>");
                    for (var index = 1; index <= pageCount; index++) {
                        navigateFuncStr = "navigateHex(" + projId + ", " + testcaseID + ", " + loopIndex + ", \"" + mode + "\", " + index + ")";
                        var offsetNum = (((index - 1) * 60) * 16).toString(16);
                        if (index == currentPage){
                            linkElem = "<li id='" + loopIndex + "link" + index + "' class='active'><a href='#' onclick='" + navigateFuncStr + "'>" + offsetNum + "</a></li>";
                        } else {
                            var style = index > 5 && index < pageCount ? "style='display:none'" : "";
                            linkElem = "<li id='" + loopIndex + "link" + index + "' " + style + "><a href='#' onclick='" + navigateFuncStr + "'>" + offsetNum + "</a></li>";
                        };

                        $("#buildLinks" + loopIndex).append(linkElem);

                        if(index == 1 && pageCount > (MARGIN + 1)) {
                            $("#buildLinks" + loopIndex).append("<li class='startDots' style='display:none;'><a href='#'>...</a></li>");
                        };
                        if(index == (pageCount-1) && pageCount > (MARGIN + 1)){
                            var liTagEndDots = pageCount <= MARGIN ? "<li class='endDots' style='display:none;'><a href='#'>...</a></li>" : "<li class='endDots'><a href='#'>...</a></li>";
                            $("#buildLinks" + loopIndex).append(liTagEndDots);
                        };
                    };
                    $("#buildLinks" + loopIndex).append("<li onclick='nextHex(" + loopIndex + ")' class='rightArrow'><a href='#' aria-label='Next'><span aria-hidden='true'>&raquo;</span></a></li>");
                    if ( parent.length > 0 ){
                        $("#jumpPagination" + loopIndex).append("<input onclick='loadHexdumpComp(" + projId + ", " + testcaseID + ", " + loopIndex + ", \"dual\")' id='compare' class='btn btn-primary' type='submit' title='" + parent + "' value='Compare with parent'>");
                    };
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

function loadHexdumpComp(projId, testcaseID, loopIndex, mode="dual", offset=0, currentPage=1, init=true, marker=-1){
    var URL = "/hexdump/" + projId + "/" + testcaseID + "/" + offset + "/1";
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
                hexDataT = response["hexT"];
                decodedDataT = response["decodedT"];
                hexDataP = response["hexP"];
                decodedDataP = response["decodedP"];
                diff = response["diff"];
                ratio = response["ratio"];
                pageCount = response["pageCount"];

                document.getElementsByClassName("offset" + loopIndex)[0].setAttribute("mode", "dual");

                // build hex elements
                $("#buildHexdump" + loopIndex).empty();
                $("#buildHexdump" + loopIndex).scrollTop(0);
                $("#buildHexdump" + loopIndex).append("<table id=\"hexTable\"><thead><tr class=\"bz\"><td colspan=\"8\" class=\"br\"></td><td class=\"br\" colspan=\"26\">Parent of Testcase</td><td colspan=\"26\">Testcase</td></tr></thead></table>");
                var hexTableHtml = $("<table id=\"hexTable\"><thead><tr class=\"by\"><td colspan=\"8\" class=\"br\">Offset(h)</td><td colspan=\"2\">00</td><td colspan=\"2\">01</td><td colspan=\"2\">02</td><td colspan=\"2\">03</td><td colspan=\"2\">04</td><td colspan=\"2\">05</td><td colspan=\"2\">06</td><td class=\"br\" colspan=\"2\">07</td><td class=\"br\" colspan=\"10\">Decoded text</td><td colspan=\"2\">00</td><td colspan=\"2\">01</td><td colspan=\"2\">02</td><td colspan=\"2\">03</td><td colspan=\"2\">04</td><td colspan=\"2\">05</td><td colspan=\"2\">06</td><td colspan=\"2\">07</td><td class=\"bp\" colspan=\"10\">Decoded text</td></tr></thead></table>");
                $("#buildHexdump" + loopIndex).append(hexTableHtml);
                $("#buildHexdump" + loopIndex).append("<table id=\"hexTable\"><thead><tr class=\"bz\"><td id=\"hovStatus\" colspan=\"8\" class=\"br\">&nbsp;</td><td class=\"br\" id=\"hovPStatus\" colspan=\"26\"></td><td id=\"hovTStatus\" colspan=\"26\"></td></tr></thead></table>");
                var hexTableBody = $("<tbody></tbody>");
                $(hexTableHtml).append(hexTableBody);
                $.each(hexDataT, function(key, row){
                    var hexTableRow = $("<tr></tr>");
                    $(hexTableBody).append(hexTableRow);
                    var offsetNum = ((key * 8) + ((currentPage - 1) * 960)).toString(16);
                    var offsetZ = offsetNum
                    var offsetZeroLength = 8 - offsetZ.length;
                    for (var z = 0; z < offsetZeroLength; z++) {
                        offsetZ = "0" + offsetZ;
                    };
                    if(marker == offsetNum){
                        var markerElement = "<td id=\"markerElement\" colspan=\"8\" class=\"bx\"> " + offsetZ + "</td>"
                        $(hexTableRow).append(markerElement);
                        document.getElementById("markerElement").scrollIntoView();
                    }else{
                        $(hexTableRow).append("<td colspan=\"8\" class=\"br\"> " + offsetZ + "</td>");
                    }

                    var hexTableDataP = "";
                    var hexTableTextP = "";
                    var hexTableDataT = "";
                    var hexTableTextT = "";
                    $.each(row, function(i, value){
                        var classNumber = String((key * 8) + i)

                        if(hexDataP[key][i].length > 0){
                            hexTableDataP = hexTableDataP.concat("<td colspan=\"2\" class=\"p" + classNumber + "\">" + hexDataP[key][i] + "</td>");
                        }else{
                            hexTableDataP = hexTableDataP.concat("<td colspan=\"2\" class=\"p" + classNumber + "\"></td>");
                        }

                        if (i==0){
                            hexTableTextP = hexTableTextP.concat("<td colspan=\"1\" class=\"bp\"></td><td colspan=\"1\" class=\"p" + classNumber + "\">");
                        }else{
                            hexTableTextP = hexTableTextP.concat("<td colspan=\"1\" class=\"p" + classNumber + "\">");
                        }
                        hexTableTextP = hexTableTextP.concat(decodedDataP[key][i] + "</td>");

                        if(value.length > 0){
                            hexTableDataT = hexTableDataT.concat("<td colspan=\"2\" class=\"t" + classNumber + "\">" + value + "</td>");
                        }else{
                            hexTableDataT = hexTableDataT.concat("<td colspan=\"2\" class=\"t" + classNumber + "\"></td>");
                        }
                        if (i==0){
                            hexTableTextT = hexTableTextT.concat("<td colspan=\"1\" class=\"bp\"></td><td colspan=\"1\" class=\"t" + classNumber + "\">");
                        }else{
                            hexTableTextT = hexTableTextT.concat("<td colspan=\"1\" class=\"t" + classNumber + "\">");
                        }
                        hexTableTextT = hexTableTextT.concat(decodedDataT[key][i] + "</td>");
                    });
                    hexTableTextT = hexTableTextT.concat("<td colspan=\"1\"></td>");
                    hexTableTextP = hexTableTextP.concat("<td colspan=\"1\" class=\"br\"></td>");
                    $(hexTableRow).append(hexTableDataP);
                    $(hexTableRow).append(hexTableTextP);
                    $(hexTableRow).append(hexTableDataT);
                    $(hexTableRow).append(hexTableTextT);
                });
                $.each(diff, function(x, value){
                    var hoverClass = "hov" + value[0].toString().charAt(0).toUpperCase() + value[0].toString().slice(1);

                    for(let i = parseInt(value[1]) - offset; i < Math.min(parseInt(value[2]) - offset, offset + 960); i++) {
                        $(".p" + i).addClass(hoverClass + x);
                    };
                    for(let i = parseInt(value[3]) - offset; i < Math.min(parseInt(value[4]) - offset, offset + 960); i++) {
                        $(".t" + i).addClass(hoverClass + x);
                    };
                });

                var hovClassList = ["hovEqual", "hovReplace", "hovDelete", "hovInsert"];
                $.each( hovClassList, function( index, value ){
                    var hovCheck = ""
                    $("td[class*='" + value + "']").on('mouseover', function() {
                        if(!$(this).hasClass('hovered')){
                            var classArray = $(this).attr('class').split(" ");
                            for(var i = 0; i < classArray.length; i++){
                                if($(this).attr('class').indexOf(value) !== -1){
                                    $('.' + classArray[i]).addClass('hovered');
                                };
                            };
                        };
                        var classArray = $(this).attr('class').split(" ");
                        for(var i = 0; i < classArray.length; i++){
                            if(/^t/.test(classArray[i]) || /^p/.test(classArray[i])){
                                if(!hovCheck){
                                    hovCheck = classArray[i];
                                    $("#hovStatus").text(value.slice(3));
                                    $.each(diff, function(x, value){
                                        var hovCheckCalc = parseInt(hovCheck.slice(1)) + ((currentPage - 1) * 960);
                                        if(/^p/.test(hovCheck)){
                                            if(parseInt(value[1]) <= hovCheckCalc && hovCheckCalc < parseInt(value[2])){
                                                $("#hovPStatus").text(parseInt(value[1]).toString(16) + " --> " + parseInt(value[2]).toString(16));
                                                $("#hovTStatus").text(parseInt(value[3]).toString(16) + " --> " + parseInt(value[4]).toString(16));
                                            };
                                        };
                                        if(/^t/.test(hovCheck)){
                                            if(parseInt(value[3]) <= hovCheckCalc && hovCheckCalc < parseInt(value[4])){
                                                $("#hovPStatus").text(parseInt(value[1]).toString(16) + " --> " + parseInt(value[2]).toString(16));
                                                $("#hovTStatus").text(parseInt(value[3]).toString(16) + " --> " + parseInt(value[4]).toString(16));
                                            };
                                        };
                                   });
                                };
                            };
                            break;
                        };
                    }).on('mouseleave', function(e) {
                        var targetOrig = e.target
                        var targetElem = e.toElement || e.relatedTarget
                        var removeCheck = false
                        var removeInfo = false
                        if ((typeof $(targetElem).attr('class') !== "undefined") && (typeof $(targetOrig).attr('class') !== "undefined")){
                            var targetOrigClassArray = $(targetOrig).attr('class').split(" ");
                            var targetElemClassArray = $(targetElem).attr('class').split(" ");
                            for(var i = 0; i < targetOrigClassArray.length; i++){
                                for(var j = 0; j < targetElemClassArray.length; j++){
                                    if((typeof targetOrigClassArray[i] !== "undefined") && (typeof targetElemClassArray[i] !== "undefined")){
                                        if(targetOrigClassArray[i].includes(value) && targetElemClassArray[i].includes(value)){
                                            if(targetOrigClassArray[i] != targetElemClassArray[i]){
                                                $("td[class*='" + targetOrigClassArray[i] + "']").removeClass('hovered');
                                                removeCheck = true;
                                                removeInfo = true;
                                            };
                                        };
                                    }else{
                                        $("td[class*='hov']").removeClass('hovered');
                                        removeInfo = true;
                                    };
                                };
                            };
                            if(removeCheck){
                                $("td[class*='hov']").removeClass('hovered');
                                removeInfo = true;
                            };
                        }else{
                            $("td[class*='hov']").removeClass('hovered');
                            removeInfo = true;
                        };
                        if(removeInfo){
                            $("#hovStatus").html('&nbsp;');
                            $("#hovPStatus").text("");
                            $("#hovTStatus").text("");
                            hovCheck = "";
                        };
                    });
                });

                if(init){
                    $("#buildLinks" + loopIndex).empty();
                    $("#compare").remove();
                    $("#buildLinks" + loopIndex).append("<li onclick='prevHex(" + loopIndex + ")' class='leftArrow'><a href='#' aria-label='Previous'><span aria-hidden='true'>&laquo;</span></a></li>");
                    for (var index = 1; index <= pageCount; index++) {
                        navigateFuncStr = "navigateHex(" + projId + ", " + testcaseID + ", " + loopIndex + ", \"" + mode + "\", " + index + ")";
                        var offsetNum = (((index - 1) * 60) * 16).toString(16);
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

function jumpToOffset(projId, testcaseID, loopIndex, offsetNum, mode){
    if(event.key === 'Enter') {
        var page = Math.floor(parseInt("0x" + offsetNum.value) / 960) + 1;
        marker = parseInt("0x" + offsetNum.value) - parseInt(parseInt("0x" + offsetNum.value) % 16);
        navigateHex(projId, testcaseID, loopIndex, mode, page, marker.toString(16));
    }
}

function navigateHex(projId, testcaseID, loopIndex, mode, currentPage, marker=-1){
    var offset = (currentPage-1) * 960;

    if(mode==="single"){
        loadHexdump(projId, testcaseID, loopIndex, mode, offset, currentPage, false, marker);
    };
    if(mode==="dual"){
        loadHexdumpComp(projId, testcaseID, loopIndex, mode, offset, currentPage, false, marker);
    };
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

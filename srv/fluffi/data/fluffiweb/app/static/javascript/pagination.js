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

var MARGIN = 5;

function loadPagination(projId, sdguid, loopIndex, offset=0, currentPage=1, init=true){
    var URL = "/projects/" + projId + "/managedInstanceLogs";
    var data = { "sdguid":  sdguid, "offset": offset, "init": init };
    var miLogs = [];    
    var pageCount = 1;
    var linkElem = "";
    var navigateFuncStr = "";

    $(".modal-loader").css("display", "inline");
    $("#buildLogs" + loopIndex).css("visibility", "hidden");

    $.ajax({
        url: URL,
        type: 'POST',
        data: JSON.stringify(data),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        success: function(response) {            
            miLogs = response["miLogs"];
            pageCount = response["pageCount"];
            // build log message elements
            $("#buildLogs" + loopIndex).empty();
            miLogs.forEach(function(log){        
                $("#buildLogs" + loopIndex).append("<p>" + log + "</p>");
            });
            
            if (miLogs.length == 0){
                $("#modalLink" + loopIndex).text("None");                               
                $("#modalLink" + loopIndex).css({ "pointer-events": "none", "cursor": "default", "color": "inherit" });
            }
            // build pagination link elements only the first time
            else if(init){
                $("#buildLinks" + loopIndex).append("<li onclick='prev(" + loopIndex + ")' class='leftArrow'><a href='#' aria-label='Previous'><span aria-hidden='true'>&laquo;</span></a></li>");                
                for (var index = 1; index <= pageCount; index++) {
                    navigateFuncStr = "navigate(" + projId + ", \"" + sdguid + "\", " + loopIndex + ", " + index + ")";
                    if (index == currentPage){
                        linkElem = "<li id='" + loopIndex + "link" + index + "' class='active'><a href='#' onclick='" + navigateFuncStr + "'>" + index + "</a></li>";                        
                    }            
                    else {
                        var style = index > 5 && index < pageCount ? "style='display:none'" : "";
                        linkElem = "<li id='" + loopIndex + "link" + index + "' " + style + "><a href='#' onclick='" + navigateFuncStr + "'>" + index + "</a></li>";
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
                $("#buildLinks" + loopIndex).append("<li onclick='next(" + loopIndex + ")' class='rightArrow'><a href='#' aria-label='Next'><span aria-hidden='true'>&raquo;</span></a></li>");
                $("#modalLink" + loopIndex).text("LogMessage(s)");
            }
            else if(pageCount > (MARGIN + 1)) {
                updatePageLinks(loopIndex);
            } 
            $(".modal-loader").css("display", "none");
            $("#buildLogs" + loopIndex).css("visibility", "visible");
        }        
    });    
}

function navigate(projId, sdguid, loopIndex, currentPage){      
    var offset = (currentPage-1) * 10;

    loadPagination(projId, sdguid, loopIndex, offset, currentPage, false);
    $("#buildLinks" + loopIndex).children().removeClass("active");
    $("#" + loopIndex + "link" + currentPage).addClass("active");
}

function next(loopIndex){
    var links = $("#buildLinks" + loopIndex).children();  
    for (var index = 1; index < links.length-1; index++) {
        var nextIndex = links.eq(index+1).hasClass("startDots") || links.eq(index+1).hasClass("endDots") ? index + 2 : index + 1;        
        if(links.eq(index).hasClass("active") && links.eq(nextIndex).length){                    
            links.eq(nextIndex).children().first().click();    
            break;                
        }               
    }    
}

function prev(loopIndex){
    var links = $("#buildLinks" + loopIndex).children();        
    for (var index = 1; index < links.length-1; index++) {
        var nextIndex = links.eq(index-1).hasClass("endDots") || links.eq(index-1).hasClass("startDots") ? index - 2 : index - 1;
        if(links.eq(index).hasClass("active") && links.eq(nextIndex).length){             
            links.eq(nextIndex).children().first().click();    
            break;                
        }        
    } 
}

function updatePageLinks(loopIndex){        
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

function staticNavigateForLMlogs(index){
    var pageId = "#page" + index;  
    var linkId = "#link" + index;

    $(".activePage").hide();
    $(".activePage").removeClass("activePage");
    $(".active").removeClass("active");

    $(pageId).show();
    $(pageId).addClass("activePage");
    $(linkId).addClass("active");
}

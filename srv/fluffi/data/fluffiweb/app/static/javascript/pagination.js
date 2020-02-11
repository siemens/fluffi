

function loadPagination(projId, sdguid, loopIndex, offset=0, currentPage=1, init=true){
    var URL = "/projects/" + projId + "/managedInstanceLogs";
    var data = { "sdguid":  sdguid, "offset": offset, "init": init };
    var miLogs = [];    
    var pageCount = 1;
    var linkElem = "";
    var navigateFuncStr = "";

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

            // build pagination link elements only the first time
            if(init){
                $("#buildLinks" + loopIndex).append("<li onclick='prev(" + loopIndex + ")' class='leftArrow'><a href='#' aria-label='Previous'><span aria-hidden='true'>&laquo;</span></a></li>");                
                for (var index = 1; index <= pageCount; index++) {
                    navigateFuncStr = "navigate(" + projId + ", \"" + sdguid + "\", " + loopIndex + ", " + index + ")";
                    if (index == currentPage){
                        linkElem = "<li id='link" + index + "' class='active'><a href='#' onclick='" + navigateFuncStr + "'>" + index + "</a></li>";                        
                    }            
                    else {
                        var style = index > 5 && index < pageCount ? "style='display:none'" : "";
                        linkElem = "<li id='link" + index + "' " + style + "><a href='#' onclick='" + navigateFuncStr + "'>" + index + "</a></li>";
                    }            
                    $("#buildLinks" + loopIndex).append(linkElem); 

                    if(index == 1) { $("#buildLinks" + loopIndex).append("<li class='startDots' style='display:none;'><a href='#'>...</a></li>"); }                  
                    if(index == (pageCount-1)){ $("#buildLinks" + loopIndex).append("<li class='endDots'><a href='#'>...</a></li>"); }         
                }                
                $("#buildLinks" + loopIndex).append("<li onclick='next(" + loopIndex + ")' class='rightArrow'><a href='#' aria-label='Next'><span aria-hidden='true'>&raquo;</span></a></li>");
            }
            else {
                updatePageLinks(loopIndex);
            }            
        }
    });         
}

function navigate(projId, sdguid, loopIndex, currentPage){      
    var offset = (currentPage-1) * 10;

    loadPagination(projId, sdguid, loopIndex, offset, currentPage, false);
    $("#buildLinks" + loopIndex).children().removeClass("active");
    $("#link" + currentPage).addClass("active");
}

function next(loopIndex){
    var links = $("#buildLinks" + loopIndex).children();        
    for (var index = 1; index < links.length-1; index++) {
        var nextIndex = links.eq(index+1).hasClass("startDots") ? index + 2 : index + 1;
        if(links.eq(index).hasClass("active") && links.eq(nextIndex).length){             
            links.eq(nextIndex).children().first().click();    
            break;                
        }        
    }    
}

function prev(loopIndex){
    var links = $("#buildLinks" + loopIndex).children();        
    for (var index = 1; index < links.length-1; index++) {
        var nextIndex = links.eq(index-1).hasClass("endDots") ? index - 2 : index - 1;
        if(links.eq(index).hasClass("active") && links.eq(nextIndex).length){             
            links.eq(nextIndex).children().first().click();    
            break;                
        }        
    } 
}

function updatePageLinks(loopIndex){
    var MARGIN = 5;
    // Used for distance from indexOfActiveLink to last element
    var distance = 0;
    var links = $("#buildLinks" + loopIndex).children();
    var indexOfActiveLink = null;
    links.each(function(i){ if($(this).hasClass("active")){ indexOfActiveLink = i; }}); 
    var endIndex = links.length-2; 
    distance = (links.length-2) - indexOfActiveLink;

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
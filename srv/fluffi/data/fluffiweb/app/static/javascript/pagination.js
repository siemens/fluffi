

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
            console.log("pageCount: ", pageCount);
            // build log message elements
            $("#buildLogs" + loopIndex).empty();
            miLogs.forEach(function(log){        
                $("#buildLogs" + loopIndex).append("<p>" + log + "</p>");
            });

            // build pagination link elements only the first time
            if(init){
                $("#buildLinks" + loopIndex).append("<li><a href='#' aria-label='Previous'><span aria-hidden='true'>&laquo;</span></a></li>");                
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
                $("#buildLinks" + loopIndex).append("<li><a href='#' aria-label='Next'><span aria-hidden='true'>&raquo;</span></a></li>");
            }
            else {
                updatePageLinks(loopIndex, currentPage, pageCount);
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

function updatePageLinks(loopIndex, currentPage, numberOfPages){
    // MARGIN needs to be greater than 4 
    var MARGIN = 6;
    var links = $("#buildLinks" + loopIndex).children();
    console.log("links: ", links);
    // TODO use length of links instead
    console.log("numberOfPages: ", numberOfPages);
    // Handle beginning of pageLinks
    if(currentPage == MARGIN){  
        links.eq(2).show();
        links.eq(3).hide();
        links.eq(4).hide();
    } else if (currentPage < MARGIN) {
        links.eq(2).hide();
        links.eq(3).show();
        links.eq(4).show();
    }    
    
    // Handle everything in between
    links.each(function(i){        
        if($(this).hasClass("active") && currentPage == 5) { 
            links.eq(i+1).show();  
        }

        if($(this).hasClass("active") && i > MARGIN){
            console.log("i: ", i); 
            links.eq(i+1).show();      
            links.eq(i+2).show();      
            links.eq(i-2).hide();      
        }                 
    });

    // Handle ending of pageLinks
    // if(currentPage == (numberOfPages - MARGIN)){  
    //     links.eq(numberOfPages-2).show();
    //     links.eq(numberOfPages-3).hide();
    //     links.eq(numberOfPages-4).hide();
    // } 
    // else if (currentPage > (numberOfPages - MARGIN)) {
    //     links.eq(numberOfPages-2).hide();
    //     links.eq(numberOfPages-3).show();
    //     links.eq(numberOfPages-4).show();
    // } 
}
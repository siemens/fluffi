
function loadPagination(projId, sdguid, loopIndex, start=0, end=10){
    var URL = "/projects/" + projId + "/managedInstanceLogs";
    var data = { "sdguid":  sdguid };

    $.ajax({
        url: URL,
        type: 'POST',
        data: JSON.stringify(data),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        success: function(response) {            
            buildPage(projId, sdguid, loopIndex, response["miLogs"], response["pageCount"]);
        }
    });
}

function buildPage(projId, sdguid, loopIndex, miLogs, pageCount){
    var linkElem = "";
    var navigateFuncStr = "";

    // build log message elements
    $("#buildLogs" + loopIndex).empty();
    miLogs.forEach(function(log){        
        $("#buildLogs" + loopIndex).append("<p>" + log + "</p>");
    });

    // build pagination link elements
    $("#buildLinks" + loopIndex).empty();
    for (var index = 1; index <= pageCount; index++) {
        navigateFuncStr = "navigate(" + projId + ", " + sdguid + ", " + index + ", " + loopIndex + ")";
        if (index == 1){
            linkElem = "<li id='link" + index + "' class='active'><a href='#' onclick='" + navigateFuncStr + "'>" + index + "</a></li>";
        }            
        else {
            linkElem = "<li id='link" + index + "'><a href='#' onclick='" + navigateFuncStr + "'>" + index + "</a></li>";
        }            
        $("#buildLinks" + loopIndex).append(linkElem);                
    }
}

function navigate(projId, sdguid, pageNumber, loopIndex){  
    var start = (pageNumber * 10) + 1;
    var end = (pageNumber * 10) + 10;
    loadPagination(projId, sdguid, loopIndex, start, end);

    $(".active").removeClass("active");
    $("#link" + pageNumber).addClass("active");
}
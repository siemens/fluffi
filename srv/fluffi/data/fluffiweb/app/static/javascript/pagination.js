
function loadPagination(projId, sdguid, loopIndex, start=0, end=10, currentPage=1){
    var URL = "/projects/" + projId + "/managedInstanceLogs";
    var data = { "sdguid":  sdguid, "start": start, "end": end };
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
        async: false,
        success: function(response) {            
            miLogs = response["miLogs"];
            pageCount = response["pageCount"];
        }
    });    

    console.log("miLogs", miLogs);
    console.log("pageCount", pageCount);
    console.log("currentPage", currentPage);

    // build log message elements
    $("#buildLogs" + loopIndex).empty();
    miLogs.forEach(function(log){        
        $("#buildLogs" + loopIndex).append("<p>" + log + "</p>");
    });

    // build pagination link elements
    $("#buildLinks" + loopIndex).empty();
    for (var index = 1; index <= pageCount; index++) {
        navigateFuncStr = "navigate(" + projId + ", " + sdguid + ", " + loopIndex + ", " + index + ")";
        if (index == currentPage){
            linkElem = "<li id='link" + index + "' class='active'><a href='#' onclick='" + navigateFuncStr + "'>" + index + "</a></li>";
        }            
        else {
            linkElem = "<li id='link" + index + "'><a href='#' onclick='" + navigateFuncStr + "'>" + index + "</a></li>";
        }            
        $("#buildLinks" + loopIndex).append(linkElem);                
    }
}


function navigate(projId, sdguid, loopIndex, currentPage){  
    console.log("currentPage: ", currentPage);
    var start = (currentPage-1) * 10;
    var end = currentPage * 10;
    console.log("start end: ", start, end);

    loadPagination(projId, sdguid, loopIndex, start, end, currentPage);

    $(".active").removeClass("active");
    $("#link" + currentPage).addClass("active");
}
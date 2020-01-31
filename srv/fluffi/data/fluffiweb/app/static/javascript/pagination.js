// Pagination for already existing data in html

function navigate(index){  
    pageId = "#page" + index;  
    linkId = "#link" + index;

    $(".activePage").hide();
    $(".activePage").removeClass("activePage");
    $(".active").removeClass("active");

    $(pageId).show();
    $(pageId).addClass("activePage");
    $(linkId).addClass("active");
}
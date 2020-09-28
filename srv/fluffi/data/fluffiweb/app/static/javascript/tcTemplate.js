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

function deleteTestcase(testcaseId, tcType) {
    var deleteUrl = "/projects/" + "{{ data.project.ID }}" + "/delTestcase/" + testcaseId + "/" + tcType;
    window.location.replace(deleteUrl);
}

function addSinglePage(i, actualPage){
    var link = ""
    var singlePage = document.createElement('div');
    $(singlePage).addClass('single_page');
    $(singlePage).html(i);
    if(i == actualPage){
        $(singlePage).addClass('single_page_active');
    }else if(i == "..."){
        $(singlePage).addClass('single_page_dots');
    }else{
        link = i.toString()
        $(singlePage).addClass('single_page_inactive');
        $(singlePage).addClass("link_" + i.toString());
    }
    $(".pages").append(singlePage);
    if($(".single_page").hasClass("link_" + i.toString())){
        $(".link_" + i.toString()).wrap('<a href="' + link + '"></a>');
    }
}

function addPages(actualPage, pageCount){
    var margin = 3;
    maxPages = actualPage + margin;
    if(maxPages > pageCount){
        maxPages = pageCount;
    }
    if(actualPage >= margin + 3){
        addSinglePage(1, actualPage);
        addSinglePage("...", actualPage);

        for(var i = actualPage - margin; i <= maxPages; i++) {
            addSinglePage(i, actualPage);
        }
    }else{
        for(var i = 1; i <= maxPages; i++) {
            addSinglePage(i, actualPage);
        }
    }
    if(actualPage < pageCount - margin){
        if(actualPage != pageCount - (margin + 1)){
            addSinglePage("...", actualPage);
        }
        addSinglePage(pageCount, actualPage);
    }
}
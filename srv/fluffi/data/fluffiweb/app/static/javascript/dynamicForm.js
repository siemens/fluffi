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

Author(s): Junes Najah, Thomas Riedmaier, Pascal Eckmann
*/


$(function() {
    $("div[data-toggle=fieldset]").each(function() {
        var $this = $(this);
            
        //Add new target
        $this.find("button[data-toggle=fieldset-add-row]").click(function() {
            var target = $($(this).data("target"))
            var oldRow = target.find("[data-toggle=fieldset-entry]:last");
            var row = oldRow.clone(true, true);
            var elemId = row.find(":input")[0].name;
            var elemNum = parseInt(elemId.replace(/.*-(\d{1,4})-.*/m, '$1')) + 1;
            var newNum = 1;
            var id = 0;
            var counter = 1;
            row.find(":input").each(function() {
                if(counter == 1){
                    if(!Number.isFinite(parseInt($(this).attr('id')))){
                        id = $(this).attr('id').replace('target_module', 1+'_targetname');
                    }else{
                        id = parseInt($(this).attr('id')) + 1 + '_targetname';
                    }
                    $(this).attr('name', id).attr('id', id).val('').removeAttr("checked");  
                }else{
                    if(!Number.isFinite(parseInt($(this).attr('id')))){
                        id = $(this).attr('id').replace('target_module_path', 1+'_targetpath');
                    }else{
                        id = parseInt($(this).attr('id')) + 1 + '_targetpath';
                    }
                    $(this).attr('name', id).attr('id', id).val('').removeAttr("checked");
                    $(this).attr('value', '*').val('*');  
                }
                counter += 1;
            });
            counter = 1;
            oldRow.after(row);
        }); //End add new target

        //Add new option
        $this.find("button[data-toggle=fieldset-add-rowoption]").click(function() {
            var target = $($(this).data("target"))
            var oldRow = target.find("[data-toggle=fieldset-entryoption]:last");
            var row = oldRow.clone(true, true);
            var elemId = row.find(":input")[0].name;
            var elemNum = parseInt(elemId.replace(/.*-(\d{1,4})-.*/m, '$1')) + 1;
            var newNum = 1;
            var id = 0;
            var counter = 1;
            row.find(":input").each(function() {
                if(counter == 1){
                    if(!Number.isFinite(parseInt($(this).attr('id')))){
                        id = $(this).attr('id').replace('option_module', 1+'_optionname');
                    }else{
                        id = parseInt($(this).attr('id')) + 1+'_optionname';
                    }  
                }else{
                    if(!Number.isFinite(parseInt($(this).attr('id')))){
                        id = $(this).attr('id').replace('option_module_value', 1+'_optionvalue');
                    }else{
                        id = parseInt($(this).attr('id')) + 1 + '_optionvalue';
                    }  
                }
                $(this).attr('name', id).attr('id', id).val('').removeAttr("checked");
                $(this).attr('placeholder', '').val('').removeAttr("checked");
                counter += 1;
            });
            counter = 1;
            oldRow.after(row);
        }); //End add new entry

        //Remove row target
        $this.find("button[data-toggle=fieldset-remove-row]").click(function() {
            if($this.find("[data-toggle=fieldset-entry]").length > 1) {
                var thisRow = $(this).closest("[data-toggle=fieldset-entry]");
                thisRow.remove();
            }
        }); //End remove row target

        //Remove row option
        $this.find("button[data-toggle=fieldset-remove-rowoption]").click(function() {
            if($this.find("[data-toggle=fieldset-entryoption]").length > 1) {
                var thisRow = $(this).closest("[data-toggle=fieldset-entryoption]");
                thisRow.remove();
            }
        }); //End remove row option
    });
});

VALID_INPUT = true;

function addTypeSetting(subType, name){
    var sum = 0;
    var errorMsgID = name == "generator" ? "#generatorTypesErrorMsg" : "#evaluatorTypesErrorMsg";
    var chosenTypesID = name == "generator" ? "#chosenGeneratorTypes": "#chosenEvaluatorTypes";
    var chosenGeneratorTypes = "";
    subType.forEach(function(type,i){ sum += parseInt($("#"+type+i).val()); });
    
    if(sum == 100){
        $(errorMsgID).hide();
        VALID_INPUT = true;
        $("#fuzzButton").attr("disabled", false);
    }else{
        VALID_INPUT = false;
        $(errorMsgID).show();
        $("#fuzzButton").attr("disabled", true);
    }

    subType.forEach(function(type,i){
        inputType = $("#"+type+i).val();
        pipe = i == subType.length-1 ? "" : "|"; 
        chosenGeneratorTypes += type + "=" + inputType + pipe;
    });

    $(chosenTypesID).html(chosenGeneratorTypes);
}

$("select.runner-type").change(function(){
    var selectedRunnerType = $(this).children("option:selected").val();
    if(selectedRunnerType == "ALL_GDB"){
        $(".basic-block-upload-box").toggleClass("basic-bock-box-visible", false);
    }else{
        $(".basic-block-upload-box").toggleClass("basic-bock-box-visible", true);
    }
}).change();

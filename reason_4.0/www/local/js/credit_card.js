/**
 *
 * Hides credit card fields for forms that offer a no payment options
 *
 * @author Steve Smith
 * @author Ben Wilbur
 * 
 * @todo add option for multiple payment amount that will add them together. Users can then create their own forms for multiple payment amount totals (i.e. addition). 
 *   Currently, these types of forms are 
 * @todo add features for all credit card payments and add to /usr/local/webapps/reason/reason_package_local/local/minisite_templates/modules/form/views/thor/credit_card_payment.php
 * @todo feature - if multipage disco, autofill credit card and billing fields with previous information (checkbox "Use same address....")
 */

$(document).ready(function() {
    
    // get the reason_id of the row containing "Payment Amount"
    var payment_row_id = ($(".words:contains('Payment Amount')").parent().attr('id'));
    var payment_row_id = payment_row_id.replace(/id/i, "id_");
    var payment_name = payment_row_id.replace(/row/i, "");
    
    // if the form creator has included a hidden field with the value 'No Payment Option' hide the credit_card_info until needed
    if ($("input[name='No Payment Option']")) {
        toggle_credit_card_info('true');
    }
    
    // if Payment Amount is already checked, send the checked button's first character to toggle_credit_card_info(). 
    if ($('input[name$="'+payment_name+'"]:checked').val()){
        toggle_credit_card_info($('input[name$="'+payment_name+'"]:checked').val().charAt(0));
    }
    
    // if Payment Amount is changed, send the checked button's first character to toggle_credit_card_info(). 
    $('input[name$="'+payment_name+'"]').change(function(){
        toggle_credit_card_info($(this).val().charAt(0));
    });
});

function toggle_credit_card_info(first_character){

    if(first_character == '$'){
        $("#paymentnoteRow").show();
        $("#creditcardtypeRow").show();
        $("#creditcardnumberRow").show();
        $("#creditcardexpirationmonthRow").show();
        $("#creditcardexpirationyearRow").show();
        $("#creditcardnameRow").show();
        $("#billingstreetaddressRow").show();
        $("#billingcityRow").show();
        $("#billingstateprovinceRow").show();
        $("#billingzipRow").show();
        $("#billingcountryRow").show();
    
    } else {
    
        $("#paymentnoteRow").hide();
        $("#creditcardtypeRow").hide();
        $("#creditcardnumberRow").hide();
        $("#creditcardexpirationmonthRow").hide();
        $("#creditcardexpirationyearRow").hide();
        $("#creditcardnameRow").hide();
        $("#billingstreetaddressRow").hide();
        $("#billingcityRow").hide();
        $("#billingstateprovinceRow").hide();
        $("#billingzipRow").hide();
        $("#billingcountryRow").hide();
    
    }
}
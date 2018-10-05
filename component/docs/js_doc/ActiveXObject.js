/**
 * @constructor
 * @param {string} name
 */
function ActiveXObject(name) {

    /**
     * Emulates COM's weird behaviour of value accessors.
     * 
     * @return {*}
     * 
     * @example
     * var saved_value = some_activex.ActiveX_Get(1, 'additional_info');
     * // in COM:
     * // var saved_value = some_activex.Item(1, 'additional_info');
     */
    this.ActiveX_Get = function () { };

    /**
     * Emulates COM's weird behaviour of value accessors.
     *
     * @example
     * some_activex.ActiveX_Set(1, "new_value", 'additional_info');
     * // in COM:
     * // some_activex.Item(1, 'additional_info') = "new_value";
     */
    this.ActiveX_Set = function () { };
}

var Util = require("../Utility.js");

module.exports = {
    callback: async function(ST, args){
        var report = [];
        var msg;
        var src;
        var smell_level;
        for(const structure_id in ST.structures){
            const structure = ST.structures[structure_id];
            for(const method_id in structure.methods){
                const method = structure.methods[method_id];

                smell_level = Util.get_smell_lvl(args.max_nested_scopes.min, args.max_nested_scopes.max, method.max_scope);
                if(smell_level > 0){ 
                    msg = `Method: "${method_id}" has ${method.max_scope} nested scopes.`;
                    src = Util.get_src_obj(method.src_info.file, method.src_info.line, method.src_info.col, structure_id, method_id);
                    report.push(Util.get_incident_obj(src, msg, smell_level));
                }
            }
        }
        return report;
    }
}
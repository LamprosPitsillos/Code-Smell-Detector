import {obs} from "../Observer/observer.js"

(async () => {
    const json = await fetch("/Config/config.json").then(response => response.json());
    
    var config = new Vue({
        el: '#config',
        data: {
            title: json.title,
            groups: json.groups
        },
        methods: {
            evalCondition(condition) {
                return eval(condition);
            }, 
            onChange(event, data) {
                obs.fire(event, data);
            }, 
            onChangeSelect(event, data) {
                obs.fire(event+data);
            }
        }, 
    });
})();


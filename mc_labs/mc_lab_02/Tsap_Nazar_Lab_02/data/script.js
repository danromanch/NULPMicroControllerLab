document.addEventListener('DOMContentLoaded', () => {
    const algo1Button = document.querySelector('#algo1');
    const algo2Button = document.querySelector('#algo2');

    function handlePress() {
        fetch('/press')
            .then(() => console.log('pressed'))
            .catch(err => console.error(err));
    }

    function handleRelease1(){
        fetch('/releaseAlgo1')
            .then(() => console.log('releaseAlgo1'))
            .catch(err => console.error(err));
    }
    
    function handleRelease2() {
        fetch('/releaseAlgo2')
            .then(() => console.log('releaseAlgo2'))
            .catch(err => console.error(err));
    }
    
    function updateStatus() {
        fetch('/status')
            .then(response => response.json())
            .then(data => {
                document.getElementById("square1").style.backgroundColor = data.red ? "red" : "white";
                document.getElementById("square2").style.backgroundColor = data.blue ? "blue" : "white";
                document.getElementById("square3").style.backgroundColor = data.green ? "green" : "white";
            })
            .catch(err => console.error(err));
    }
    
    setInterval(updateStatus, 100);

    algo1Button.addEventListener('mousedown', handlePress);
    algo1Button.addEventListener('touchstart', handlePress);
    algo1Button.addEventListener('mouseup', handleRelease1);
    algo1Button.addEventListener('touchend', handleRelease1);
    algo2Button.addEventListener('mouseup', handleRelease2);
    algo2Button.addEventListener('touchend', handleRelease2);
});



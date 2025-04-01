document.addEventListener('DOMContentLoaded', () => {
    const myButton = document.querySelector('#my_button');
    const NazarButton = document.querySelector('#nazar_button');

    function handleMyRelease() {
        fetch('/release')
            .then(() => console.log('Released'))
            .catch(err => console.error(err));
    }

    function handleNazarPress(){
        fetch('/pressNazar')
            .then(() => console.log('Pressed'))
            .catch(err => console.error(err));
    }
    function handleNazarRelease() {
        fetch('/releaseNazar')
            .then(() => console.log('releaseNazar'))
            .catch(err => console.error(err));
    }

    function getLEDStatus() {
        fetch('/ledstatus')
            .then(response => response.json())
            .then(data => {
                document.getElementById("sq1").style.backgroundColor = data.blue ? "blue" : "white";
                document.getElementById("sq2").style.backgroundColor = data.yellow ? "yellow" : "white";
                document.getElementById("sq3").style.backgroundColor = data.red ? "red" : "white";
            })
            .catch(err => console.error(err));
    }

    setInterval(getLEDStatus, 100);

    NazarButton.addEventListener('mousedown', handleNazarPress);
    NazarButton.addEventListener('touchstart', handleNazarPress);
    NazarButton.addEventListener('mouseup', handleNazarRelease);
    NazarButton.addEventListener('touchend', handleNazarRelease);
    myButton.addEventListener('mouseup', handleMyRelease);
    myButton.addEventListener('touchend', handleMyRelease);
});
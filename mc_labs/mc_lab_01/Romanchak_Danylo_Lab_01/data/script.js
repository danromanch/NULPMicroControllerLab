document.addEventListener('DOMContentLoaded', () => {
    const btn = document.querySelector('.button');

    function handleRelease() {
        fetch('/release')
            .then(() => console.log('Released'))
            .catch(err => console.error(err));
    }

    btn.addEventListener('mouseup', handleRelease);
    btn.addEventListener('touchend', handleRelease);
});
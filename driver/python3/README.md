## Build soure distribution

```
python3 setup.py sdist
```

## Upload source distribution

```
python3 -m twine upload --repository-url https://test.pypi.org/legacy/ dist/*
```

## Install on target

```
sudo pip3 install -i https://test.pypi.org/simple/ --upgrade timeswipe
```

## Documentation:

```
pydoc3 timeswipe
```

## Test:

```
sudo python3 test.py
```

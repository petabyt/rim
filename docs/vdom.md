# Virtual DOM
*Has nothing to do with the [DOM](https://en.wikipedia.org/wiki/Document_Object_Model) of course.*

Essentially Rim has to work with 3 different trees:
- New Tree
- Old Tree
- UI Tree in the backend (abstracted away through append/remove/setOnClick/create functions)

The tree differ is given 2 trees:
- The old tree
- The new tree

The old tree represents what the UI currently looks like. Each node in this tree has a pointer to
the backend toolkit widget handle which has all its properties exactly as the node describes.

The new tree represents what the UI needs to look like. None of the nodes have a pointer to
a backend toolkit widget handle.

It's up to the tree differ to transform the backend UI tree from what it currently is (old tree)
into what it needs to be (new tree). Sometimes this means changing properties, sometimes it means destroying
or creating layouts and attaching them to a window.

In the React world, this process is known as reconciliation.
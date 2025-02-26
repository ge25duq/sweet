#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import numpy as np
from scipy.linalg import lu

try:
    # Relative import (when used as a package module)
    from .nodes import NodesGenerator
    from .lagrange import LagrangeApproximation
except ImportError:
    # Absolute import (when used as script)
    from nodes import NodesGenerator
    from lagrange import LagrangeApproximation

from mule.SWEETFileDict import SWEETFileDict

# Storage for diagonaly optimized QDelta matrices
OPT_COEFFS = {
    # Fun fact :
    # the sum of the diagonal coefficients that minimize the spectral radius of Q-Q_Delta is equal to :
    # 
    #                            nCoeffs/order
    # 
    # - nCoeffs : number of non-zeros coefficients
    # - order : collocation order
    #
    # In particular :
    # - GAUSS :       nCoeffs(M) = M,   order(M) = 2M   => sum = 1/2
    # - RADAU-LEFT :  nCoeffs(M) = M-1, order(M) = 2M-1 => sum = (M-1)/(2M-1)
    # - RADAU-RIGHT : nCoeffs(M) = M,   order(M) = 2M-1 => sum = M/(2M-1)
    # - LOBATTO :     nCoeffs(M) = M-1, order(M) = 2M-2 => sum = 1/2
    "QMQD": {
        2: {'GAUSS':
                [(0.105662, 0.394338),  # sum = 1/2
                 (0.394338, 0.105662)], # sum = 1/2
            'RADAU-LEFT':
                [(0.0, 0.333333)],      # sum = 1/3 -> (0, 1/3)
            'RADAU-RIGHT':
                [(0.166667, 0.5),       # sum = 2/3 -> (1/6, 1/2)
                 (0.666667, 0.0)],      # sum = 2/3 -> (2/3, 0)
            'LOBATTO':
                [(0.0, 0.5)]            # sum = 1/2 -> (0, 1/2)
            },
        3: {'GAUSS':
                [(0.037571, 0.166670, 0.295770),   # sum = 1/2
                 (0.156407, 0.076528, 0.267066),   # sum = 1/2
                 (0.267065, 0.076528, 0.156407),   # sum = 1/2
                 (0.295766, 0.166666, 0.037567)],  # sum = 1/2
            'RADAU-LEFT':
                [(0.0, 0.118350, 0.281650),        # sum = 2/5
                 (0.0, 0.322474, 0.077526)],       # sum = 2/5
            'RADAU-RIGHT':
                [(0.051682, 0.214981, 0.333333),   # sum = 3/5 -> (11/210, 3/14, 1/3)
                 (0.233475, 0.080905, 0.285619),   # sum = 3/5
                 (0.390077, 0.094537, 0.115385),   # sum = 3/5
                 (0.422474, 0.177525, 0.0)],       # sum = 3/5
            'LOBATTO':
                [(0.0, 0.166667, 0.333333),        # sum = 1/2
                 (0.0, 0.5, 0.0)],                 # sum = 1/2
            },
        4: {'GAUSS':
                [(0.077743, 0.044737, 0.155099, 0.222721),  # sum = 1/2
                 (0.131879, 0.039977, 0.121777, 0.204056),  # sum = 1/2
                 (0.163870, 0.108119, 0.043928, 0.184203),  # sum = 1/2
                 (0.204603, 0.122324, 0.040523, 0.132426)], # sum = 1/2
            'RADAU-LEFT':
                [(0.0, 0.053082, 0.147630, 0.227850),       # sum = 3/7,
                 (0.0, 0.158930, 0.065439, 0.204202),       # sum = 3/7
                 (0.0, 0.241626, 0.068757, 0.118188),       # sum = 3/7
                 (0.0, 0.262554, 0.136489, 0.029530)],      # sum = 3/7
            'RADAU-RIGHT':
                [(0.179657, 0.047644, 0.134350, 0.209484),  # sum = 4/7,
                 (0.243813, 0.054458, 0.117399, 0.156936),  # sum = 4/7
                 (0.273342, 0.152240, 0.036186, 0.110861),  # sum = 4/7
                 (0.303862, 0.196902, 0.070838, 0.0)],      # sum = 4/7
            'LOBATTO':
                [(0.0, 0.069096, 0.180900, 0.249998),       # sum = 1/2
                 (0.0, 0.225168, 0.063361, 0.211471),       # sum = 1/2
                 (0.0, 0.338774, 0.077899, 0.083337),       # sum = 1/2
                 (0.0, 0.361804, 0.138197, 0.0)],           # sum = 1/2
            },
        5: {'RADAU-RIGHT':
                [(0.193913, 0.141717, 0.071975, 0.018731, 0.119556),  # sum = 5/9
                 (0.205563, 0.143134, 0.036388, 0.073742, 0.10488),   # sum = 5/9
                 (0.176822, 0.124251, 0.031575, 0.084012, 0.142621)], # sum = 5/9
            'LOBATTO':
                [(0.0, 0.035130, 0.100594, 0.166059, 0.200594),
                 (0.0, 0.115624, 0.046456, 0.149272, 0.186773),
                 (0.0, 0.179692, 0.046428, 0.108742, 0.164978),
                 (0.0, 0.252532, 0.132452, 0.029955, 0.085783)]
            },

        },
    "SPECK": {
        2: {'GAUSS':
                [(0.166667, 0.5),
             	 (0.5, 0.166667)],
            'RADAU-LEFT':
                [(0.0, 0.333333)],
            'RADAU-RIGHT':
                [(0.258418, 0.644949),
                 (1.074915, 0.155051)],
            'LOBATTO':
                [(0.0, 0.5)]
            },
        3: {'GAUSS':
                [(0.07672, 0.258752, 0.419774),
                 (0.214643, 0.114312, 0.339631),
                 (0.339637, 0.114314, 0.214647),
                 (0.419779, 0.258755, 0.076721)],
            'RADAU-RIGHT':
                [(0.10405, 0.332812, 0.48129),
                 (0.320383, 0.139967, 0.371668), # Winner for advection
                 (0.558747, 0.136536, 0.218466),
                 (0.747625, 0.404063, 0.055172)],
            'LOBATTO':
                [(0.0, 0.211325, 0.394338),
                 (0.0, 0.788675, 0.105662)]
            },
        },
    "NR": {
        2: {'GAUSS':
                [(0.25, 0.25)],
            'RADAU-LEFT':
                [(0.0, 0.333333)],
            'RADAU-RIGHT':
                [(0.416666, 0.25)],
            'LOBATTO':
                [(0.0, 0.5)],
            },
        3: {'GAUSS':
                [(0.152083, 0.279867, 0.198563),
                 (0.328062, 0.273704, 0.167562),
                 (0.177932, 0.470270, -0.043369)],
            'RADAU-LEFT':
                [(0.0, 0.220412, 0.179587)],
            'RADAU-RIGHT':
                [(0.331304, 0.229214, 0.248398),
                 (0.366962, 0.408431, 0.134487)],
            'LOBATTO':
                [(0.0, 0.333333, 0.166666)],
            },
        5: {'RADAU-RIGHT':
                [(0.328759, 0.345207, 0.352007, 0.265738, 0.040074),
                 (0.354490, 0.316147, 0.251464, 0.294285, 0.128222),
                 (0.351177, 0.299790, 0.300124, 0.255459, 0.165975)],
            'LOBATTO':
                [(0.0, 0.258511, 0.264935, 0.308452, 0.204955),
                 (0.0, 0.302692, 0.239282, 0.306889, 0.201339),
                 (0.0, 0.331331, 0.309104, 0.237257, 0.165573),
                 (0.0, 0.304966, 0.284686, 0.283417, 0.173670)]
            },
        },
        "ADAPT": {
            3: {'RADAU-RIGHT':
                [(0.6032189, 0.071476, 0.186304),
                 (0.8616106, 0.341044, -0.02394),]
            },
            5: {'RADAU-RIGHT':
                [(0.060094, 0.026725, 0.068137, 0.113716, 0.141697),
                 (-0.262690, 0.053929, 0.089201, 0.127347, 0.153412),
                 (-0.023117, 0.113279, 0.011166, 0.078374, 0.115008),]
            }
        }
    }

# Coefficient allowing A-stability with prolongation=True
WEIRD_COEFFS = {
    'GAUSS':
        {2: (0.5, 0.5)},
    'RADAU-RIGHT':
        {2: (0.5, 0.5)},
    'RADAU-LEFT':
        {3: (0.0, 0.5, 0.5)},
    'LOBATTO':
        {3: (0.0, 0.5, 0.5)}}

def genQDelta(nodes, sweepType, Q):
    """
    Generate QDelta matrix for a given node distribution

    Parameters
    ----------
    nodes : array (M,)
        quadrature nodes, scaled to [0, 1]
    sweepType : str
        Type of sweep, that defines QDelta. Can be selected from :
    - BE : Backward Euler sweep (first order)
    - FE : Forward Euler sweep (first order)
    - LU : uses the LU trick
    - TRAP : sweep based on Trapezoidal rule (second order)
    - EXACT : don't bother and just use Q
    - PIC : Picard iteration => zeros coefficient
    - OPT-[...] : Diagonaly precomputed coefficients, for which one has to
      provide different parameters. For instance, [...]='QmQd-2' uses the
      diagonal coefficients using the optimization method QmQd with the index 2
      solution (index starts at 0 !). Quadtype and number of nodes are
      determined automatically from the Q matrix.
    - WEIRD-[...] : diagonal coefficient allowing A-stability with collocation
      update (forceProl=True).
    Q : array (M,M)
        Q matrix associated to the node distribution
        (used only when sweepType in [LU, EXACT, OPT-[...], WEIRD]).

    Returns
    -------
    QDelta : array (M,M)
        The generated QDelta matrix.
    dtau : float
        Correction coefficient for time integration with QDelta
    """
    # Generate deltas
    deltas = np.copy(nodes)
    deltas[1:] = np.ediff1d(nodes)

    # Extract informations from Q matrix
    M = deltas.size
    if sweepType.startswith('OPT') or sweepType == 'WEIRD':
        leftIsNode = np.allclose(Q[0], 0)
        rightIsNode = np.isclose(Q[-1].sum(), 1)
        quadType = 'LOBATTO' if (leftIsNode and rightIsNode) else \
            'RADAU-LEFT' if leftIsNode else \
            'RADAU-RIGHT' if rightIsNode else \
            'GAUSS'

    # Compute QDelta
    QDelta = np.zeros((M, M))
    dtau = 0.0
    if sweepType in ['BE', 'FE']:
        offset = 1 if sweepType == 'FE' else 0
        for i in range(offset, M):
            QDelta[i:, :M-i] += np.diag(deltas[offset:M-i+offset])
        if sweepType == 'FE':
            dtau = deltas[0]
    elif sweepType == 'TRAP':
        for i in range(0, M):
            QDelta[i:, :M-i] += np.diag(deltas[:M-i])
        for i in range(1, M):
            QDelta[i:, :M-i] += np.diag(deltas[1:M-i+1])
        QDelta /= 2.0
        dtau = deltas[0]/2.0
    elif sweepType == 'LU':
        QT = Q.T.copy()
        [_, _, U] = lu(QT, overwrite_a=True)
        QDelta = U.T
    elif sweepType == 'EXACT':
        QDelta = np.copy(Q)
    elif sweepType == 'PIC':
        QDelta = np.zeros(Q.shape)
    elif sweepType.startswith('OPT'):
        try:
            oType, idx = sweepType[4:].split('-')
        except ValueError:
            raise ValueError(f'missing parameter(s) in sweepType={sweepType}')
        M, idx = int(M), int(idx)
        try:
            coeffs = OPT_COEFFS[oType][M][quadType][idx]
            QDelta[:] = np.diag(coeffs)
        except (KeyError, IndexError):
            raise ValueError('no OPT diagonal coefficients for '
                             f'{oType}-{M}-{quadType}-{idx}')
    elif sweepType == 'BEPAR':
        QDelta[:] = np.diag(nodes)
    elif sweepType == 'WEIRD':
        try:
            coeffs = WEIRD_COEFFS[quadType][M]
            QDelta[:] = np.diag(coeffs)
        except (KeyError, IndexError):
            raise ValueError('no WEIRD diagonal coefficients for '
                             f'{M}-{quadType} nodes')
    else:
        raise NotImplementedError(f'sweepType={sweepType}')
    return QDelta, dtau


def genCollocation(M, distr, quadType):
    """
    Generate the nodes, weights and Q matrix for a given collocation method

    Parameters
    ----------
    M : int
        Number of quadrature nodes.
    distr : str
        Node distribution. Can be selected from :
    - LEGENDRE : nodes from the Legendre polynomials
    - EQUID : equidistant nodes distribution
    - CHEBY-{1,2,3,4} : nodes from the Chebyshev polynomial (1st to 4th kind)
    quadType : str
        Quadrature type. Can be selected from :
    - GAUSS : do not include the boundary points in the nodes
    - RADAU-LEFT : include left boundary points in the nodes
    - RADAU-RIGHT : include right boundary points in the nodes
    - LOBATTO : include both boundary points in the nodes

    Returns
    -------
    nodes : array (M,)
        quadrature nodes, scaled to [0, 1]
    weights : array (M,)
        quadrature weights associated to the nodes
    Q : array (M,M)
        normalized Q matrix of the collocation problem
    """

    # Generate nodes between [0, 1]
    nodes = NodesGenerator(node_type=distr, quad_type=quadType).getNodes(M)
    nodes += 1
    nodes /= 2
    np.round(nodes, 14, out=nodes)

    # Compute Q and weights
    approx = LagrangeApproximation(nodes)
    Q = approx.getIntegrationMatrix([(0, tau) for tau in nodes])
    weights = approx.getIntegrationMatrix([(0, 1)]).ravel()

    return nodes, weights, Q


def getSetup(
    nNodes:int=3, nodeType:str='RADAU-RIGHT', nIter:int=3, 
    qDeltaImplicit:str='BE', qDeltaExplicit:str='FE',
    diagonal:bool=False, initialSweepType:str='COPY', useEndUpdate:bool=False,
    qDeltaInitial:str='BEPAR', nodeDistr:str='LEGENDRE'
    )-> SWEETFileDict:
    """
    Generate SWEETFileDict for one given SDC setup

    Parameters
    ----------
    nNodes : int, optional
        Number of nodes.
    nodeType : str, optional
        Quadrature type for the nodes, can be 'GAUSS', 'LOBATTO', 'RADAU-RIGHT' or 'RADAU-LEFT'. 
    nIter : int, optional
        Number of iterations (sweeps).
    qDeltaImplicit : str, optional
        Base (implicit) sweep for SDC. Can be 'BE', 'BEpar', 'LU', ... (see genQDelta doc). 
    qDeltaExplicit : str, optional
        Explicit sweep (when used for IMEX SDC). Can be 'FE', 'PIC', ... (see genQDelta doc). 
    diagonal : bool, optional
        Wether or not use the diagonal implementation. Warning : should be used
        with compatible QDelta matrices.
    initialSweepType : str, optional
        The way the tendencies are initialized before the first sweep. Can be :

        - COPY : use the time-step initial solution to evaluate the tendencies,
          and copy those tendencies for each nodes.
        - QDELTA : use the QDelta matrices (implicit and explicit) to compute
          one IMEX update between the nodes and evaluate the tendencies from
          those. If diagonal=True, then uses the values provided by the
          qDeltaInitial parameter.

    useEndUpdate : bool, optional
        Wether or not compute the end update with the quadrature formula.
    qDeltaInitial : str, optional
        Diagonal sweep used if diagonal=True and initialSweepType=QDELTA, must be a diagonal sweep.
    nodeDistr : str, optional
        Node distribution.
        
    Example
    -------
    >>> # Settings for Fast Wave Slow Wave IMEX SDC
    >>> paramsSDC = genSetup()
    >>> # Settings for Optimized Parallel SDC
    >>> paramsSDC = genSetup(
    >>>     qDeltaImplicit='OPT-QmQd-0', qDeltaExplicit='PIC', diagonal=True,
    >>>     initialSweepType='QDELTA', qDeltaInitial='BEpar')

    Returns
    -------
    out : SWEETFileDict
        The SDC parameters and coefficients, with the following keys :
            
        - idString : str
            Unique string identifier.
        - nodes : nd.1darray(M)
            Node values (between [0, 1]).
        - weights : nd.1darray(M)
            Weight values.
        - qMatrix : nd.2darray(M,M)
            Q matrix coefficients.
        - qDeltaI : nd.2darray(M,M)
            Implicit sweep coefficients.
        - qDeltaE : nd.2darray(M,M)
            Explicit sweep coefficients.
        - qDelta0 : nd.2darray(M,M)
            Initial sweep coefficients.
        - initialSweepType : str
            Type of initial sweep.
        - diagonal : int
            1 if use the diagonal implementation, 0 else.
        - useEndUpdate : int
            1 if use the end update, 0 else.
    """
    nodes, weights, qMatrix = genCollocation(nNodes, nodeDistr, nodeType)
    qDeltaI = genQDelta(nodes, qDeltaImplicit, qMatrix)[0]
    qDeltaE = genQDelta(nodes, qDeltaExplicit, qMatrix)[0]
    qDelta0 = genQDelta(nodes, qDeltaInitial, qMatrix)[0]
    idString = f'M{nNodes}_{nodeType}_K{nIter}_{qDeltaImplicit}_{qDeltaExplicit}_{initialSweepType}'
    out = SWEETFileDict(initDict={
        'nodes': nodes,
        'weights': weights,
        'qMatrix': qMatrix,
        'qDeltaI': qDeltaI,
        'qDeltaE': qDeltaE,
        'qDelta0': qDelta0,
        'initialSweepType': initialSweepType,
        'nIter': nIter
    })
    out['diagonal'] = int(diagonal)
    if initialSweepType == 'QDELTA' and diagonal:
        idString += f'_{qDeltaInitial}'
    out['useEndUpdate'] = int(useEndUpdate)
    if useEndUpdate:
        idString += '_endUpdate'
    if diagonal:
        idString += '_diag'
    out['idString'] = idString
    return out
    
